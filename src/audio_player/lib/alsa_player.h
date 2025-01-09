// A class to encapsulate playing an AudioFile with ALSA.
// Takes shared ownership of an AudioFile that it will play.
//
// Created by sean on 1/9/25.
//

#ifndef ALSA_PLAYER_H
#define ALSA_PLAYER_H

#include "audio_player.h"

#include <alsa/asoundlib.h>
#include <fmt/core.h>

#include <atomic>
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
    AlsaPlayer(PlaybackState &inState): mState(inState) {
    };

    bool init(const std::shared_ptr<AudioFile> &inFile) {
        mAudioFile = inFile;

        unsigned int channels = inFile->channels();
        unsigned int rate = inFile->sampleRate();

        mFileInfo = {.mNumChannels = channels, .mSampleRate = rate};

        return init(channels, rate);
    }

    void printInfo() const {
        // ---------------------------------
        // Output some hardware information.

        fmt::print("PCM name: '{}'\n", snd_pcm_name(mPcmHandle));
        fmt::print("PCM state: {}\n", snd_pcm_state_name(snd_pcm_state(mPcmHandle)));

        unsigned int hwChannels;
        snd_pcm_hw_params_get_channels(mParams, &hwChannels);
        fmt::print("channels: {}\n", hwChannels);

        unsigned int hwRate;
        snd_pcm_hw_params_get_rate(mParams, &hwRate, 0);
        fmt::print("Hardware rate: {} HZ\n", hwRate);
    }

    bool play() {
        if (!mAudioFile || !mPcmHandle || !mParams || !mAudioFile) {
            return false;
        }

        // -----------------------------
        // Create buffer for stdin data.

        snd_pcm_uframes_t framesPerPeriod;
        snd_pcm_hw_params_get_period_size(mParams, &framesPerPeriod, 0);

        fmt::print("Hardware period size: {} frames\n", framesPerPeriod);

        // -----------------------------------
        // Play file data received from stdin.

        unsigned int hwPeriodTime;
        snd_pcm_hw_params_get_period_time(mParams, &hwPeriodTime, nullptr);

        // Period time is apparently in microseconds.
        fmt::print("Hardware period time: {} us\n", hwPeriodTime);

        float *fileData = mAudioFile->data();

        std::size_t samplesPerPeriod = framesPerPeriod * mFileInfo.mNumChannels;
        // NOTE: This could potentially truncate audio by very small amount.
        std::size_t numPeriods = mAudioFile->dataLength() / samplesPerPeriod;

        fmt::print("\nPlaying sound data from file...\n");

        mState.mPlaying = true;
        for (std::size_t i = 0; i < numPeriods && mState.mPlaying; i++) {
            // NOTE: This knows how many bytes each frame contains.
            // This will buffer frames for playback by the sound card.
            // Evidence suggests we stay well ahead of playback here.
            snd_pcm_sframes_t framesWritten = snd_pcm_writei(mPcmHandle, fileData, framesPerPeriod);

            if (framesWritten == -EPIPE) {
                // An underrun has occurred, which happens when "an application
                // does not feed new samples in time to alsa-lib (due CPU usage)".
                fmt::print("An underrun has occurred while writing to device.\n");

                snd_pcm_prepare(mPcmHandle);
            } else if (framesWritten < 0) {
                // The docs say this could be -EBADFD or -ESTRPIPE.
                fmt::print("Failed to write to PCM device: {}\n", snd_strerror(static_cast<int>(framesWritten)));
            }

            // Increment data pointer to start of next frame.
            fileData += samplesPerPeriod;
        }

        return true;
    }

    void shutdown() {
        // -----------------------
        // Clean up and shut down.

        snd_pcm_drain(mPcmHandle);
        snd_pcm_close(mPcmHandle);

        fmt::print("Playback complete.\n");
    }

private:
    bool init(unsigned int numChannels, unsigned int sampleRate) {
        // ---------------
        // Setup ALSA PCM.

        fmt::print("Preparing ALSA...\n\n");

        // Try opening the device.
        //
        // NOTE: Mode 0 is the default BLOCKING mode.
        // So our calls to snd_pcm_writei below will block
        // until all frames sent are played or buffered.

        int pcmResult = snd_pcm_open(&mPcmHandle, PCM_DEVICE, SND_PCM_STREAM_PLAYBACK, 0);

        if (pcmResult < 0) {
            fmt::print("Can't open PCM device '{}': {}\n", PCM_DEVICE, snd_strerror(pcmResult));

            return false;
        }

        // ----------------------------
        // Configure hardware settings.

        // Allocate object & get defaults.
        snd_pcm_hw_params_alloca(&mParams);
        snd_pcm_hw_params_any(mPcmHandle, mParams);

        pcmResult = snd_pcm_hw_params_set_access(mPcmHandle, mParams, SND_PCM_ACCESS_RW_INTERLEAVED);

        if (pcmResult < 0) {
            fmt::print("Failed to set access mode: {}\n", snd_strerror(pcmResult));

            return true;
        }

        // Format "signed 16 bit Little Endian".
        pcmResult = snd_pcm_hw_params_set_format(mPcmHandle, mParams, SND_PCM_FORMAT_FLOAT_LE);

        if (pcmResult < 0) {
            fmt::print("Failed to set format: {}\n", snd_strerror(pcmResult));

            return true;
        }

        pcmResult = snd_pcm_hw_params_set_channels(mPcmHandle, mParams, numChannels);

        if (pcmResult < 0) {
            fmt::print("Failed to set number of channels: {}\n", snd_strerror(pcmResult));

            return true;
        }

        pcmResult = snd_pcm_hw_params_set_rate_near(mPcmHandle, mParams, &sampleRate, 0);

        if (pcmResult < 0) {
            fmt::print("Failed to set rate: {}\n", snd_strerror(pcmResult));

            return true;
        }

        pcmResult = snd_pcm_hw_params(mPcmHandle, mParams);

        if (pcmResult < 0) {
            fmt::print("Failed to set hardware params: {}\n", snd_strerror(pcmResult));

            return true;
        }

        return true;
    }

private:
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
