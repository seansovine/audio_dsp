// A class to encapsulate playing an AudioFile with ALSA.
// Takes shared ownership of an AudioFile that it will play.
//
// Created by sean on 1/9/25.

#ifndef ALSA_PLAYER_H
#define ALSA_PLAYER_H

#include "audio_player.h"

#include <atomic>
#include <memory>

// ----------------------------
// State shared across threads.

struct SharedPlaybackState {
    std::atomic_bool mPlaying;
    std::atomic<float> mAvgIntensity;
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

    // Get some ALSA config information.
    void getInfo(AlsaInfo *info) const;

    bool play();

    // Clean up and close handle.
    void shutdown();

  private:
    // Setup ALSA PCM.
    bool initPcm(unsigned int numChannels, unsigned int sampleRate);

    int setBufferSize();

  private:
    SharedPlaybackState &mState;
    std::shared_ptr<const AudioFile> mAudioFile;

    struct FileInfo {
        unsigned int mNumChannels = 0;
        unsigned int mSampleRate = 0;
    };
    FileInfo mFileInfo;
};

#endif // ALSA_PLAYER_H
