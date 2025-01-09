// A class to encapsulate playing an AudioFile with ALSA.
// Takes shared ownership of an AudioFile that it will play.
//
// Created by sean on 1/9/25.
//

#ifndef ALSA_PLAYER_H
#define ALSA_PLAYER_H

#include "audio_player.h"
#include "fmt/printf.h"

#include <alsa/asoundlib.h>

#include <atomic>
#include <format>
#include <memory>

// ----------------------------
// State shared across threads.

struct PlaybackState {
    std::atomic_bool mPlaying;
};

// -----------------------------------------
// Class for playing an AudioFile with ALSA.

class AlsaPlayer {
    static constexpr auto PCM_DEVICE = "default";

public:
    AlsaPlayer(PlaybackState &inState, MessageQueue &inQueue): mLogger(
            Logger{inQueue}), mState(inState) {};

    bool init(const std::shared_ptr<AudioFile> &inFile) {
        mAudioFile = inFile;

        unsigned int channels = inFile->channels();
        unsigned int rate = inFile->sampleRate();

        mFileInfo = {.mNumChannels = channels, .mSampleRate = rate};

        return init(channels, rate);
    }

    void printInfo() {
        // ---------------------------------
        // Output some hardware information.

        log("PCM name: '{}'\n", snd_pcm_name(mPcmHandle));
        log("PCM state: {}\n",
                   snd_pcm_state_name(snd_pcm_state(mPcmHandle)));

        unsigned int hwChannels;
        snd_pcm_hw_params_get_channels(mParams, &hwChannels);
        log("channels: {}\n", hwChannels);

        unsigned int hwRate;
        snd_pcm_hw_params_get_rate(mParams, &hwRate, 0);
        log("Hardware rate: {} HZ\n", hwRate);
    }

    bool play() {
        if (!mAudioFile || !mPcmHandle || !mParams || !mAudioFile) {
            return false;
        }

        // -----------------------------
        // Create buffer for stdin data.

        snd_pcm_uframes_t framesPerPeriod;
        snd_pcm_hw_params_get_period_size(mParams, &framesPerPeriod, 0);

        log("Hardware period size: {} frames\n", framesPerPeriod);

        // -----------------------------------
        // Play file data received from stdin.

        unsigned int hwPeriodTime;
        snd_pcm_hw_params_get_period_time(mParams, &hwPeriodTime, nullptr);

        // Period time is apparently in microseconds.
        log("Hardware period time: {} us\n", hwPeriodTime);

        float *fileData = mAudioFile->data();

        std::size_t samplesPerPeriod = framesPerPeriod * mFileInfo.mNumChannels;
        // NOTE: This could potentially truncate audio by very small amount.
        std::size_t numPeriods = mAudioFile->dataLength() / samplesPerPeriod;

        log("\nPlaying sound data from file...\n");

        mState.mPlaying = true;
        for (std::size_t i = 0; i < numPeriods && mState.mPlaying; i++) {
            // NOTE: This knows how many bytes each frame contains.
            // This will buffer frames for playback by the sound card;
            // see notes in setBufferSize() definition below.
            snd_pcm_sframes_t framesWritten = snd_pcm_writei(
                mPcmHandle, fileData, framesPerPeriod);

            if (framesWritten == -EPIPE) {
                // An underrun has occurred, which happens when "an application
                // does not feed new samples in time to alsa-lib (due CPU usage)".
                log(
                    "An underrun has occurred while writing to device.\n");

                snd_pcm_prepare(mPcmHandle);
            } else if (framesWritten < 0) {
                // The docs say this could be -EBADFD or -ESTRPIPE.
                log("Failed to write to PCM device: {}\n",
                           snd_strerror(static_cast<int>(framesWritten)));
            }

            // Increment data pointer to start of next frame.
            fileData += samplesPerPeriod;
        }

        log("Playback complete.\n");

        return true;
    }

    void shutdown() {
        // -----------------------
        // Clean up and shut down.

        snd_pcm_drain(mPcmHandle);
        snd_pcm_close(mPcmHandle);
    }

private:
    // Wraps the C++20 std::format interface, for
    // use with our shared logging message queue.
    template<typename ... Args>
    void log(std::string_view fmt, Args&&... args) {
        mLogger.log(std::vformat(fmt, std::make_format_args(args...)));
    }

    bool init(unsigned int numChannels, unsigned int sampleRate) {
        // ---------------
        // Setup ALSA PCM.

        log("Preparing ALSA...\n\n");

        // Try opening the device.
        //
        // NOTE: Mode 0 is the default BLOCKING mode.
        // So our calls to snd_pcm_writei below will block
        // until all frames sent are played or buffered.

        int pcmResult = snd_pcm_open(&mPcmHandle, PCM_DEVICE,
                                     SND_PCM_STREAM_PLAYBACK, 0);

        if (pcmResult < 0) {
            log("Can't open PCM device '{}': {}\n", PCM_DEVICE,
                       snd_strerror(pcmResult));

            return false;
        }

        // ----------------------------
        // Configure hardware settings.

        // Allocate object & get defaults.
        snd_pcm_hw_params_alloca(&mParams);
        snd_pcm_hw_params_any(mPcmHandle, mParams);

        pcmResult = snd_pcm_hw_params_set_access(
            mPcmHandle, mParams, SND_PCM_ACCESS_RW_INTERLEAVED);

        if (pcmResult < 0) {
            log("Failed to set access mode: {}\n",
                       snd_strerror(pcmResult));

            return false;
        }

        // Format "signed 16 bit Little Endian".
        pcmResult = snd_pcm_hw_params_set_format(
            mPcmHandle, mParams, SND_PCM_FORMAT_FLOAT_LE);

        if (pcmResult < 0) {
            log("Failed to set format: {}\n", snd_strerror(pcmResult));

            return false;
        }

        pcmResult = snd_pcm_hw_params_set_channels(
            mPcmHandle, mParams, numChannels);

        if (pcmResult < 0) {
            log("Failed to set number of channels: {}\n",
                       snd_strerror(pcmResult));

            return false;
        }

        pcmResult = snd_pcm_hw_params_set_rate_near(
            mPcmHandle, mParams, &sampleRate, 0);

        if (pcmResult < 0) {
            log("Failed to set rate: {}\n", snd_strerror(pcmResult));

            return false;
        }

        pcmResult = setBufferSize();

        if (pcmResult < 0) {
            log("Failed to set hardware params: {}\n",
                       snd_strerror(pcmResult));

            return false;
        }

        pcmResult = snd_pcm_hw_params(mPcmHandle, mParams);

        if (pcmResult < 0) {
            log("Failed to set hardware params: {}\n",
                       snd_strerror(pcmResult));

            return false;
        }

        return true;
    }

    int setBufferSize() {
        // Set the buffer size here in order to reduce the latency in
        // sending/receiving real-time info to/from the playback loop.
        //
        // When the state of shared variables changes, the playback loop starts
        // using the new state in its real-time sample processing, and this
        // affects the output as soon as previously buffered data has been played.
        // Reducing the ALSA buffer size prevents buffering sample data too far
        // ahead of the samples currently being played, so the effects of
        // parameter changes are heard sooner, and it similarly allows sharing
        // statistics on recently played samples by updating shared variables.

        // The intent here for the maximum latency due to buffering in the real-time
        // playback loop to be 1 / latencyFactor seconds. We may have to adjust this
        // factor to strike a balance between communication latency and playback
        // smoothness, if we do much heavy real-time processing in the loop.
        //
        // NOTE: This assumes the sample rate is independent of # channels.
        constexpr unsigned int latencyFactor = 10;

        snd_pcm_uframes_t bufferSize = mFileInfo.mSampleRate / latencyFactor;

        return snd_pcm_hw_params_set_buffer_size_max(
            mPcmHandle, mParams, &bufferSize);
    }

private:
    Logger mLogger;
    PlaybackState &mState;

    snd_pcm_t *mPcmHandle = nullptr;
    snd_pcm_hw_params_t *mParams = nullptr;

    std::shared_ptr<AudioFile> mAudioFile;

    struct FileInfo {
        unsigned int mNumChannels = 0;
        unsigned int mSampleRate = 0;
    };

    FileInfo mFileInfo;
};

#endif //ALSA_PLAYER_H
