// A class to encapsulate playing an AudioFile with ALSA.
// Takes shared ownership of an AudioFile that it will play.
//
// Created by sean on 1/9/25.
//

#ifndef ALSA_PLAYER_H
#define ALSA_PLAYER_H

#include "audio_player.h"

#include <atomic>
#include <format>
#include <memory>

// ----------------------------
// State shared across threads.

struct PlaybackState {
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
    explicit AlsaPlayer(PlaybackState &inState);

    bool init(const std::shared_ptr<AudioFile> &inFile);

    // Get some ALSA config information.
    void getInfo(AlsaInfo *info) const;

    bool play();

    // Clean up and close handle.
    void shutdown();

  private:
    template <typename... Args> void log(Args &&..._) {
        // TODO: Just a placeholder until we figure out how to log
        // from this file. I.e., we figure out our strange memory
        // manipulation bugs when using ALSA.
    }

    // Setup ALSA PCM.
    bool init(unsigned int numChannels, unsigned int sampleRate);

    int setBufferSize();

  private:
    PlaybackState &mState;

    std::shared_ptr<AudioFile> mAudioFile;

    struct FileInfo {
        unsigned int mNumChannels = 0;
        unsigned int mSampleRate = 0;
    };

    FileInfo mFileInfo;
};

#endif // ALSA_PLAYER_H
