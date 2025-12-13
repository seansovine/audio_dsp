// Main audio player application class.
//
// Created by sean on 1/10/25.

#ifndef AUDIO_PLAYER_APP_H
#define AUDIO_PLAYER_APP_H

#include "alsa_player.hpp"
#include "audio_player.hpp"
#include "processing_thread.hpp"
#include "root_directory.h"
#include "rt_queue.hpp"

#include <iostream>
#include <map>
#include <optional>
#include <stdexcept>
#include <thread>

// ----------------------------------
// For managing the state of the app.

enum class State {
    NoFile,
    FileLoad,
    FilenameInput,
    Stopped,
    Playing,
};

inline std::string stateString(State state) {
    switch (state) {
    case State::NoFile: {
        return "NoFile";
    }
    case State::FileLoad: {
        return "FileLoad";
    }
    case State::FilenameInput: {
        return "FilenameInput";
    }
    case State::Stopped: {
        return "Stopped";
    }
    case State::Playing: {
        return "Playing";
    }
    default: {
        throw std::runtime_error("stateString received unhandled state.");
    }
    }
}

enum class KeyEvent {
    KEY_d,
    KEY_f,
    KEY_l,
    KEY_p,
    KEY_q,
    KEY_s,
    UNRECOGNIZED_KEY
};

struct AppState {
    AppState(ProcQueue procQueue, MainQueue mainQueue)
        : mPlaybackState(procQueue),
          mProcThreadState(mainQueue, procQueue, mProcThreadRunning){};

    State mCurrentState = State::NoFile;

    // file
    std::string mFilepath;
    std::shared_ptr<const AudioFile> mAudioFile = nullptr;

    // playback thread state
    std::atomic_bool mPlaybackInProgress = false;
    std::shared_ptr<std::thread> mPlaybackThread;

    MessageQueue mQueue;
    SharedPlaybackState mPlaybackState;

    // processing thread state
    std::atomic_bool mProcThreadRunning = false;
    ProcessingThread mProcThreadState;
    std::shared_ptr<std::thread> mProcessingThread;
};

// ----------------
// Playback thread.

// Encapsulates setting up and running the playback thread.

class PlaybackThread {
  public:
    explicit PlaybackThread(AppState &appState)
        : mLogger(Logger{appState.mQueue}),
          mPlaybackState(appState.mPlaybackState),
          mPlaybackInProgress(appState.mPlaybackInProgress),
          mAudioFile(appState.mAudioFile) {
    }

    void run() {
        AlsaPlayer player{mPlaybackState};

        if (!player.init(mAudioFile)) {
            std::cerr << "AlsaPlayer init failed." << std::endl;
            // TODO: Better error handling.
        }

        if (!player.play()) {
            std::cerr << "AlsaPlayer play failed." << std::endl;
        }

        player.shutdown();
        mPlaybackInProgress = false;
    }

  private:
    Logger mLogger;
    SharedPlaybackState &mPlaybackState;
    std::atomic_bool &mPlaybackInProgress;
    std::shared_ptr<const AudioFile> mAudioFile;
};

// -------------
// Audio player.

// Manages the underlying app state. This should
// be agnostic to the specific UI implementation.

class AudioPlayer {
    DataQueue<alsa_player::PROCESSING_WINDOW_SIZE> mProcQueue;
    DataQueue<proc_thread::NUM_SPECTROGRAM_BINS> mMainQueue;

    AppState mAppState;
    bool mRunning = true;

  public:
    AudioPlayer()
        : mProcQueue{QUEUE_CAP},
          mMainQueue{QUEUE_CAP},
          mAppState{QueueHolder{mProcQueue}, QueueHolder{mMainQueue}} {};

    AppState &appState() {
        return mAppState;
    }

    State currentState() const {
        return mAppState.mCurrentState;
    }

    bool fileIsLoaded() const {
        return mAppState.mCurrentState > State::FileLoad;
    }

    bool running() const {
        return mRunning;
    }

    bool loadAudioFile(std::optional<std::string> filePath) {
        // Hard-coded test file for quick testing. TODO: Remove later.
        static const auto testFilename = std::string(project_root) + "/media/Low E.wav";
        std::string inFilename = testFilename;

        if (filePath.has_value()) {
            inFilename = *filePath;
        }
        try {
            auto inFile = std::make_shared<AudioFile>(inFilename);

            mAppState.mAudioFile = std::move(inFile);
            mAppState.mFilepath = inFilename;
            mAppState.mCurrentState = State::Stopped;

            return true;
        } catch (const std::exception &e) {
            mAppState.mCurrentState = State::NoFile;

            return false;
        }
    }

    void loadUserAudioFile(
        const std::string &filePath,
        const std::function<void(bool, std::optional<unsigned int> channels)> &callback) {

        bool success = false;
        std::optional<unsigned int> channels = std::nullopt;

        if (filePath.empty()) {
            mAppState.mCurrentState = State::NoFile;
        } else {
            success = loadAudioFile(filePath);
            channels = mAppState.mAudioFile->channels();
        }

        callback(success, channels);
    }

    void playAudioFile() {
        mAppState.mCurrentState = State::Playing;
        mAppState.mPlaybackInProgress = true;

        mAppState.mProcThreadRunning = true;
        mAppState.mProcessingThread = std::make_shared<std::thread>(mAppState.mProcThreadState);

        mAppState.mPlaybackThread = std::make_shared<std::thread>([this]() {
            PlaybackThread pbThread{mAppState};
            pbThread.run();
        });
    }

    State handleEvent(KeyEvent event) {
        // NOTE: This could be a just switch statement.
        auto handler = sKeyHandlers[currentState()];
        (this->*handler)(event);

        return currentState();
    }

    void handleEventNoFile(KeyEvent event) {
        switch (event) {
        case KeyEvent::KEY_l: {
            mAppState.mCurrentState = State::FileLoad;
            break;
        }
        default: {
            handleEventGeneric(event);
            break;
        }
        }
    }

    void handleEventFileLoad(KeyEvent event) {
        switch (event) {
        case KeyEvent::KEY_d: {
            loadAudioFile(std::nullopt);
            break;
        }
        case KeyEvent::KEY_f: {
            // Signal to main loop to get input from user.
            mAppState.mCurrentState = State::FilenameInput;
            break;
        }
        default: {
            handleEventGeneric(event);
            break;
        }
        }
    }

    void handleEventStopped(KeyEvent event) {
        switch (event) {
        case KeyEvent::KEY_p: {
            playAudioFile();
            break;
        }
        default: {
            handleEventGeneric(event);
            break;
        }
        }
    }

    void handleEventPlaying(KeyEvent event) {
        switch (event) {
        case KeyEvent::KEY_s: {
            mAppState.mPlaybackState.mPlaying = false;
            // TODO: Maybe add code to stop and resume thread.
            shutdownPlaybackThread();
            resetPlaybackStates();
            break;
        }
        default: {
            handleEventGeneric(event);
            break;
        }
        }
    }

    void handleEventGeneric(KeyEvent event) {
        switch (event) {
        case KeyEvent::KEY_q: {
            if (currentState() == State::Playing) {
                mAppState.mPlaybackState.mPlaying = false;
                shutdownPlaybackThread();
            }
            mRunning = false;
            break;
        }
        default: {
            // Key event not handled in current state.
            break;
        }
        }
    }

    // Performs any updates due do asynchronous operations like playback.
    // Returns true if the screen needs cleared due to state update.
    bool updateState() {
        bool stateChangedUpdateNeeded = false;

        if (mAppState.mCurrentState == State::Playing && !mAppState.mPlaybackInProgress) {
            shutdownPlaybackThread();
            stateChangedUpdateNeeded = true;
        }

        return stateChangedUpdateNeeded;
    }

  private:
    void shutdownPlaybackThread() {
        mAppState.mPlaybackThread->join();
        mAppState.mPlaybackThread = nullptr;
        mAppState.mProcThreadRunning = false;
        mAppState.mProcessingThread->join();
        mAppState.mProcessingThread = nullptr;
        mAppState.mCurrentState = State::Stopped;
    }

    void resetPlaybackStates() {
        mAppState.mPlaybackState.mAvgIntensity = 0.0;
        mAppState.mPlaybackState.mNumTicks = 0;
        mAppState.mPlaybackState.mTickNum = 0;
    }

  private:
    using KeyHandler = decltype(&AudioPlayer::handleEventGeneric);
    static std::map<State, KeyHandler> sKeyHandlers;
};

// Allows dispatching to method to handle events based on state.
inline std::map<State, AudioPlayer::KeyHandler> AudioPlayer::sKeyHandlers = {
    {State::NoFile, &AudioPlayer::handleEventNoFile},
    {State::FileLoad, &AudioPlayer::handleEventFileLoad},
    {State::Stopped, &AudioPlayer::handleEventStopped},
    {State::Playing, &AudioPlayer::handleEventPlaying},
};

#endif // AUDIO_PLAYER_APP_H
