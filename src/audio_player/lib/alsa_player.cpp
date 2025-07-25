// ALSA playback interface implementation.
//
// Created by sean on 1/9/25.

#include "alsa_player.h"

#include <cmath>
#include <cstddef>
#include <cstdlib>

AlsaPlayer::AlsaPlayer(SharedPlaybackState &inState)
    : mState(inState){};

bool AlsaPlayer::init(const std::shared_ptr<const AudioFile> &inFile) {
    mAudioFile = inFile;

    unsigned int channels = inFile->channels();
    unsigned int rate = inFile->sampleRate();

    mFileInfo = {.mNumChannels = channels, .mSampleRate = rate};

    return initPcm(channels, rate);
}

// Get some ALSA config information.
// NOTE: This is currently unused.
void AlsaPlayer::getInfo(AlsaInfo *info, snd_pcm_hw_params_t *mParams) const {
    const char *_name = snd_pcm_name(mPcmHandle);
    const char *_state = snd_pcm_state_name(snd_pcm_state(mPcmHandle));

    unsigned int hwChannels;
    snd_pcm_hw_params_get_channels(mParams, &hwChannels);
    unsigned int hwRate;
    snd_pcm_hw_params_get_rate(mParams, &hwRate, 0);

    info->mNumChannels = hwChannels;
    info->mSampleRate = hwRate;
}

bool AlsaPlayer::play() {
    if (!mAudioFile || !mPcmHandle || !mAudioFile) {
        return false;
    }

    const float *fileData = mAudioFile->data();

    std::size_t samplesPerPeriod = mFramesPerPeriod * mFileInfo.mNumChannels;
    // NOTE: This could potentially truncate audio by very small amount.
    std::size_t numPeriods = mAudioFile->dataLength() / samplesPerPeriod;

    // We will try computing statistics 100 times per second.
    const unsigned int statSamplingInterval = mFileInfo.mSampleRate / (100 * mFramesPerPeriod);
    float runningAvg = 0.0;

    // ---------------
    // Real-time loop.

    mState.mPlaying = true;
    mState.mNumTicks = numPeriods;
    mState.mTickNum = 0;

    for (std::size_t i = 0; i < numPeriods && mState.mPlaying; i++) {
        // NOTE: This knows how many bytes each frame contains.
        // This will buffer frames for playback by the sound card;
        // see notes in setBufferSize() definition below.
        snd_pcm_sframes_t framesWritten = snd_pcm_writei(mPcmHandle, fileData, mFramesPerPeriod);

        if (framesWritten == -EPIPE) {
            // An underrun has occurred, which happens when "an application
            // does not feed new samples in time to alsa-lib (due CPU usage)".
            //
            // log("An underrun has occurred while writing to device.\n");

            snd_pcm_prepare(mPcmHandle);
        } else if (framesWritten < 0) {
            // The docs say this could be -EBADFD or -ESTRPIPE.
            //
            // log("Failed to write to PCM device: {}\n",
            //     snd_strerror(static_cast<int>(framesWritten)));
        }

        if (i % statSamplingInterval == 0) {
            // Positive to avoid -inf from log.
            float frameAvg = 1.0;
            for (std::size_t i = 0; i < samplesPerPeriod; i++) {
                frameAvg += fileData[i] * fileData[i];
            }
            // Avgerage with RMS volume in decibels.
            runningAvg = 0.6 * runningAvg + 0.4 * 10 * std::log(frameAvg);

            mState.mAvgIntensity = runningAvg;
            mState.mTickNum = i;
        }

        // Increment data pointer to start of next frame.
        fileData += samplesPerPeriod;
    }
    mState.mPlaying = false;

    return true;
}

// Clean up and close handle.
void AlsaPlayer::shutdown() {
    snd_pcm_close(mPcmHandle);
}

// Setup ALSA PCM.
bool AlsaPlayer::initPcm(unsigned int numChannels, unsigned int sampleRate) {
    // Try opening the device.
    //
    // NOTE: Mode 0 is the default BLOCKING mode.
    // So our calls to snd_pcm_writei below will block
    // until all frames sent are played or buffered.

    int pcmResult = snd_pcm_open(&mPcmHandle, PCM_DEVICE, SND_PCM_STREAM_PLAYBACK, 0);

    if (pcmResult < 0) {
        return false;
    }

    snd_pcm_hw_params_t *mParams = nullptr;

    // Allocate params object (on stack) & get defaults.
    snd_pcm_hw_params_alloca(&mParams);
    snd_pcm_hw_params_any(mPcmHandle, mParams);

    pcmResult = snd_pcm_hw_params_set_access(mPcmHandle, mParams, SND_PCM_ACCESS_RW_INTERLEAVED);

    if (pcmResult < 0) {
        return false;
    }

    // From pcm header: Float 32 bit Little Endian, Range -1.0 to 1.0.
    pcmResult = snd_pcm_hw_params_set_format(mPcmHandle, mParams, SND_PCM_FORMAT_FLOAT_LE);

    if (pcmResult < 0) {
        return false;
    }

    pcmResult = snd_pcm_hw_params_set_channels(mPcmHandle, mParams, numChannels);

    if (pcmResult < 0) {
        return false;
    }

    pcmResult = snd_pcm_hw_params_set_rate_near(mPcmHandle, mParams, &sampleRate, 0);

    if (pcmResult < 0) {
        return false;
    }

    pcmResult = setBufferSize(mParams);

    if (pcmResult < 0) {
        return false;
    }

    pcmResult = snd_pcm_hw_params(mPcmHandle, mParams);

    if (pcmResult < 0) {
        return false;
    }

    snd_pcm_hw_params_get_period_size(mParams, &mFramesPerPeriod, 0);
    snd_pcm_hw_params_get_period_time(mParams, &mHwPeriodTime, nullptr);

    // NOTE: clang address sanitizer says there's a (~3k) memory leak
    // originating here. It may be the stack-allocated alloca memory
    // that is fooling it, but not sure.

    return true;
}

int AlsaPlayer::setBufferSize(snd_pcm_hw_params_t *mParams) {
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

    // NOTE: This assumes the sample rate is independent of # channels.
    constexpr unsigned int latencyFactor = 10;
    snd_pcm_uframes_t bufferSize = mFileInfo.mSampleRate / latencyFactor;

    return snd_pcm_hw_params_set_buffer_size_max(mPcmHandle, mParams, &bufferSize);
}
