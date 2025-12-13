// A class to encapsulate playing an AudioFile with ALSA.
// Takes shared ownership of an AudioFile that it will play.
//
// Created by sean on 1/9/25.

#ifndef ALSA_PLAYER_H
#define ALSA_PLAYER_H

#include "audio_player.hpp"
#include "rt_queue.hpp"

#include <alsa/asoundlib.h>

#include <atomic>
#include <cstddef>
#include <memory>

// -------------------------
// Configuration parameters.

namespace alsa_player {

static constexpr size_t PROCESSING_WINDOW_SIZE = 512;

using AlsaData = Data<PROCESSING_WINDOW_SIZE>;
using AlsaDataQueue = QueueHolder<PROCESSING_WINDOW_SIZE>;

} // namespace alsa_player

// ----------------------------
// State shared across threads.

struct SharedPlaybackState {
    SharedPlaybackState(alsa_player::AlsaDataQueue &inProcQueue)
        : mProcQueue(inProcQueue){};

    std::atomic_bool mPlaying;
    std::atomic<float> mAvgIntensity;
    std::atomic<std::size_t> mTickNum;
    std::atomic<std::size_t> mNumTicks;

    alsa_player::AlsaDataQueue mProcQueue;
};

// -----------------------------------------------
// For getting ALSA info w/out dynamic allocation.

struct AlsaInfo {
    char mName[128] = {0};
    char mState[128] = {0};
    unsigned int mNumChannels = 0;
    unsigned int mSampleRate = 0;
};

// -----------------------------------------
// Class for playing an AudioFile with ALSA.

class AlsaPlayer {
    static constexpr auto PCM_DEVICE = "default";

  public:
    explicit AlsaPlayer(SharedPlaybackState &inState);

    bool init(const std::shared_ptr<const AudioFile> &inFile);

    // Get some ALSA config information. Currently unused.
    void getInfo(AlsaInfo *info, snd_pcm_hw_params_t *mParams) const;

    bool play();

    // Clean up and close handle.
    void shutdown();

  private:
    // Setup ALSA PCM.
    bool initPcm(unsigned int numChannels, unsigned int sampleRate);

    int setBufferSize(snd_pcm_hw_params_t *mParams);

  private:
    SharedPlaybackState &mState;
    std::shared_ptr<const AudioFile> mAudioFile;

    struct FileInfo {
        unsigned int mNumChannels = 0;
        unsigned int mSampleRate = 0;
    };
    FileInfo mFileInfo;

    // ALSA state params
    snd_pcm_t *mPcmHandle = nullptr;
    snd_pcm_uframes_t mFramesPerPeriod;
    unsigned int mHwPeriodTime;
};

#endif // ALSA_PLAYER_H
