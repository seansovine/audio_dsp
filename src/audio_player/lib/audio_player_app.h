// Main audio player application class.
//
// Created by sean on 1/10/25.

#ifndef AUDIO_PLAYER_APP_H
#define AUDIO_PLAYER_APP_H

#include "alsa_player.h"
#include "audio_player.h"
#include "root_directory.h"

#include <fmt/color.h>

#include <iostream>
#include <map>
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
    State mCurrentState = State::NoFile;

    // file
    std::string mFilepath;
    std::shared_ptr<const AudioFile> mAudioFile = nullptr;

    // playback thread
    std::atomic_bool mPlaybackInProgress = false;
    std::shared_ptr<std::thread> mPlaybackThread;

    // state management
    MessageQueue mQueue;
    SharedPlaybackState mPlaybackState;
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
        // TODO: Send messages for player to display.
        // mLogger.log("Playing sound data from file...");

        AlsaPlayer player{mPlaybackState};
        if (!player.init(mAudioFile)) {
            std::cerr << "AlsaPlayer init failed." << std::endl;
            // TODO: Better error handling.
        }

        // TODO
        // AlsaInfo info;
        // player.getInfo(&info);

        if (!player.play()) {
            std::cerr << "AlsaPlayer play failed." << std::endl;
            // TODO: Better error handling.
        }

        player.shutdown();
        // mLogger.log("Playback complete.");

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
  public:
    AudioPlayer() = default;

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

    void loadUserAudioFile(const std::string &filePath, const std::function<void(bool)> &callback) {
        bool success = false;

        if (filePath.empty()) {
            mAppState.mCurrentState = State::NoFile;
        } else {
            success = loadAudioFile(filePath);
        }

        callback(success);
    }

    void playAudioFile() {
        mAppState.mCurrentState = State::Playing;
        mAppState.mPlaybackInProgress = true;

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
            shutdownPlaybackThread();
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
        mAppState.mCurrentState = State::Stopped;
    }

  private:
    AppState mAppState;
    bool mRunning = true;

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
