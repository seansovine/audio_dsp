// ALSA playback interface implementation.
//
// Created by sean on 1/9/25.

#include "alsa_player.hpp"

#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdlib>

AlsaPlayer::AlsaPlayer(SharedPlaybackState &inState)
    : mState(inState){};

bool AlsaPlayer::init(const std::shared_ptr<const AudioFile> &inFile) {
    mAudioFile = inFile;

    unsigned int channels = inFile->channels();
    unsigned int rate = inFile->sampleRate();

    // These should be the only options we see,
    // but we need to check out assumptions.
    assert(channels == 1 || channels == 2);

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
    using namespace alsa_player;

    if (!mAudioFile || !mPcmHandle || !mAudioFile) {
        return false;
    }

    const float *fileData = mAudioFile->data();

    std::size_t samplesPerPeriod = mFramesPerPeriod * mFileInfo.mNumChannels;
    // NOTE: This could potentially truncate audio by small amount.
    std::size_t numPeriods = mAudioFile->dataLength() / samplesPerPeriod;

    // Compute intensity on each buffer write.
    const unsigned int statSamplingInterval = 1;
    float runningAvg = 0.0;

    // How many PCM samples per processing window.
    std::size_t dataSamplesPerWindow = alsa_player::PROCESSING_WINDOW_SIZE * mFileInfo.mNumChannels;

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

        // Update running sound intensity estimate.
        if (i % statSamplingInterval == 0) {
            // Positive to avoid -inf from log.
            float frameAvg = 1.0;
            for (std::size_t j = 0; j < samplesPerPeriod; j++) {
                frameAvg += fileData[j] * fileData[j];
            }
            // Avgerage with RMS volume in decibels.
            runningAvg = 0.6 * runningAvg + 0.4 * 10 * std::log(frameAvg);

            mState.mAvgIntensity = runningAvg;
            mState.mTickNum = i;
        }

        // Compute information relevant to data sampling.
        std::size_t currentSample = i * samplesPerPeriod;
        std::size_t dataSample = (currentSample - dataSamplesPerWindow / 2) / dataSamplesPerWindow;
        bool shouldSample =
            (currentSample >= dataSamplesPerWindow / 2) &&
            ((currentSample - dataSamplesPerWindow / 2) % dataSamplesPerWindow == 0) &&
            (currentSample + dataSamplesPerWindow / 2 < mAudioFile->dataLength());

        // Send window of data samples to processing thread.
        if (shouldSample && dataSample > 0) {
            AlsaData newData{};
            const float *sampleData = fileData - dataSamplesPerWindow / 2;
            // NOTE: This could be done more simply, by just sharing the data pointer
            // offset, since all threads access the data read-only. But, this is also
            // used as a protoype for other real-time processing that we might do in
            // the future, where we will do more than simply copy data.
            for (std::size_t j = 0; j < dataSamplesPerWindow; j++) {
                if (mFileInfo.mNumChannels == 1) {
                    newData.data[j] = sampleData[j];
                } else if (mFileInfo.mNumChannels == 2) {
                    newData.data[j / 2] = sampleData[j] + sampleData[j + 1];
                }
            }
            // Drop data and move on if queue is full.
            bool _ = mState.mProcQueue.queueRef.try_push(newData);
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
    constexpr unsigned int latencyFactor = 200;
    snd_pcm_uframes_t bufferSize = 512; // mFileInfo.mSampleRate / latencyFactor;
    // TODO: Find better way to set this based on audio sample rate.

    return snd_pcm_hw_params_set_buffer_size_max(mPcmHandle, mParams, &bufferSize);
}
