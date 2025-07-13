//
// Created by sean on 1/9/25.

#include "alsa_player.h"

#include <alsa/asoundlib.h>

// NOTE: We use static variables to help isolation the
// ALSA dependency, so clients of AlsaPlayer don't need
// to depend on it. However, this means there should only
// be one instance of AlsaPlayer (which makes sense).

snd_pcm_t *mPcmHandle = nullptr;
snd_pcm_hw_params_t *mParams = nullptr;

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
void AlsaPlayer::getInfo(AlsaInfo *info) const {
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
    if (!mAudioFile || !mPcmHandle || !mParams || !mAudioFile) {
        return false;
    }

    snd_pcm_uframes_t framesPerPeriod;
    snd_pcm_hw_params_get_period_size(mParams, &framesPerPeriod, 0);

    unsigned int hwPeriodTime;
    snd_pcm_hw_params_get_period_time(mParams, &hwPeriodTime, nullptr);

    const float *fileData = mAudioFile->data();

    std::size_t samplesPerPeriod = framesPerPeriod * mFileInfo.mNumChannels;
    // NOTE: This could potentially truncate audio by very small amount.
    std::size_t numPeriods = mAudioFile->dataLength() / samplesPerPeriod;

    // We will try computing statistics 100 times per second.
    const unsigned int statSamplingInterval = mFileInfo.mSampleRate / (100 * framesPerPeriod);
    float runningAvg = 0.0;

    // ---------------
    // Real-time loop.

    mState.mPlaying = true;
    for (std::size_t i = 0; i < numPeriods && mState.mPlaying; i++) {
        // NOTE: This knows how many bytes each frame contains.
        // This will buffer frames for playback by the sound card;
        // see notes in setBufferSize() definition below.
        snd_pcm_sframes_t framesWritten = snd_pcm_writei(mPcmHandle, fileData, framesPerPeriod);

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
            runningAvg = 0.6 * runningAvg + 0.4 * std::abs(fileData[0]);
            mState.mAvgIntensity = runningAvg;
        }

        // Increment data pointer to start of next frame.
        fileData += samplesPerPeriod;
    }

    return true;
}

// Clean up and close handle.
void AlsaPlayer::shutdown() {
    snd_pcm_drain(mPcmHandle);
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

    // Allocate params object & get defaults.
    snd_pcm_hw_params_alloca(&mParams);
    snd_pcm_hw_params_any(mPcmHandle, mParams);

    pcmResult = snd_pcm_hw_params_set_access(mPcmHandle, mParams, SND_PCM_ACCESS_RW_INTERLEAVED);

    if (pcmResult < 0) {
        return false;
    }

    // Format "signed 16 bit Little Endian".
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

    pcmResult = setBufferSize();

    if (pcmResult < 0) {
        return false;
    }

    pcmResult = snd_pcm_hw_params(mPcmHandle, mParams);

    if (pcmResult < 0) {
        return false;
    }

    return true;
}

int AlsaPlayer::setBufferSize() {
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
