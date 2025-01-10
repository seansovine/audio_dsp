//
// Created by sean on 1/10/25.
//

#ifndef AUDIO_PLAYER_APP_H
#define AUDIO_PLAYER_APP_H

// clang-format off
#include "alsa_player.h"
#include "audio_player.h"
#include "curses_console.h"
#include "root_directory.h"

#include <fmt/color.h>

#include <chrono>
#include <map>
#include <thread>
// clang-format on

// ----------------------------------
// For managing the state of the app.

enum class State {
    NoFile,
    FileLoad,
    FilenameInput,
    Stopped,
    Playing,
};

enum class KeyEvent { KEY_d, KEY_f, KEY_l, KEY_p, KEY_q, KEY_s, UNRECOGNIZED_KEY };

struct AppState {
    State mCurrentState = State::NoFile;

    std::string mFilepath;
    std::shared_ptr<AudioFile> mAudioFile = nullptr;

    std::atomic_bool mPlaybackInProgress = false;
    std::shared_ptr<std::thread> mPlaybackThread;

    MessageQueue mQueue;
    PlaybackState mPlaybackState;
};

// ----------------
// Playback thread.

// Encapsulates setting up and running the playback thread.

class PlaybackThread {
public:
    explicit PlaybackThread(AppState &appState)
        : mLogger(Logger{appState.mQueue}), mPlaybackState(appState.mPlaybackState),
          mPlaybackInProgress(appState.mPlaybackInProgress), mAudioFile(appState.mAudioFile) {}

    void run() {
        mLogger.log("Playing sound data from file...");

        AlsaPlayer player{mPlaybackState};
        player.init(mAudioFile);

        AlsaInfo info;
        player.getInfo(&info);

        player.play();
        player.shutdown();

        mLogger.log("Playback complete.");

        mPlaybackInProgress = false;
    }

private:
    Logger mLogger;
    PlaybackState &mPlaybackState;
    std::atomic_bool &mPlaybackInProgress;

    std::shared_ptr<AudioFile> mAudioFile;
};

// -------------
// Audio player.

// Manages the underlying app state. This should
// be agnostic to the specific UI implementation.

class AudioPlayer {
public:
    AudioPlayer() = default;

    AppState &appState() { return mAppState; }

    State currentState() const { return mAppState.mCurrentState; }

    bool fileIsLoaded() const { return mAppState.mCurrentState > State::FileLoad; }

    bool running() const { return mRunning; }

    bool loadAudioFile(const std::string &filePath) {
        static const auto testFilename = std::string(project_root) + "/media/Low E.wav";
        std::string inFilename = testFilename;

        if (!filePath.empty()) {
            inFilename = filePath;
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
        bool success = loadAudioFile(filePath);
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
        // NOTE: This should probably just be a switch statement.
        // It's not that efficiency is a problem here, but that it
        // would just be clearer and simpler, so more maintainable.
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

        default:
            handleEventGeneric(event);
            break;
        }
    }

    void handleEventFileLoad(KeyEvent event) {
        switch (event) {
        case KeyEvent::KEY_d: {
            loadAudioFile("");
            break;
        }

        case KeyEvent::KEY_f: {
            // Signal to main loop to get input from user.
            mAppState.mCurrentState = State::FilenameInput;
            break;
        }

        default:
            handleEventGeneric(event);
            break;
        }
    }

    void handleEventStopped(KeyEvent event) {
        switch (event) {
        case KeyEvent::KEY_p: {
            playAudioFile();
            break;
        }

        default:
            handleEventGeneric(event);
            break;
        }
    }

    void handleEventPlaying(KeyEvent event) {
        switch (event) {
        case KeyEvent::KEY_s: {
            mAppState.mPlaybackState.mPlaying = false;
            break;
        }

        default:
            handleEventGeneric(event);
            break;
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

        default:
            // Received a key not handled in current state.
            break;
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
std::map<State, AudioPlayer::KeyHandler> AudioPlayer::sKeyHandlers = {
    {State::NoFile, &AudioPlayer::handleEventNoFile},
    {State::FileLoad, &AudioPlayer::handleEventFileLoad},
    {State::Stopped, &AudioPlayer::handleEventStopped},
    {State::Playing, &AudioPlayer::handleEventPlaying},
};

// ----------------
// Console manager.

// Interface between curses console and audio player state.
// This should read, but never modify, the audio player state.

class ConsoleManager {
public:
    ConsoleManager(CursesConsole &console, AudioPlayer &player)
        : mConsole(console), mAudioPlayer(player) {}

    void showFileStatus() {
        if (mAudioPlayer.fileIsLoaded()) {
            auto msg = fmt::format("Audio file loaded: {}", mAudioPlayer.appState().mFilepath);
            mConsole.addString(msg);
        } else {
            mConsole.addString("Audio file not loaded.");
        }
        incCurrentLine(1);

        if (mAudioPlayer.currentState() == State::Playing) {
            mConsole.addString("File is playing.");
            incCurrentLine(1);
        }
        incCurrentLine(1);
    }

    void showHeader() {
        mCurrentLine = 0;
        incCurrentLine(0);

        mConsole.addString("> Simple ALSA Audio Player <");
        incCurrentLine(2);
    }

    void showOptions() {
        switch (mAudioPlayer.currentState()) {
        case State::NoFile: {
            mConsole.addString("Press l to load file.");
            incCurrentLine(1);
            break;
        }

        case State::FileLoad: {
            mConsole.addString("Press f to enter path or d to load sample audio file.");
            incCurrentLine(1);
            break;
        }

        case State::Stopped: {
            mConsole.addString("Press p to play file.");
            incCurrentLine(1);
            break;

            // TODO: Add option to change file.
        }

        case State::Playing: {
            mConsole.addString("Press s to stop playing.");
            incCurrentLine(1);
            break;
        }

        default:
            throw std::logic_error("Invalid state in showOptions.");
        }

        mConsole.addString("Press q to exit.");
    }

    std::string getFilename() {
        incCurrentLine(2);
        mConsole.addString("Enter filename to load: ");

        auto filename = mConsole.getString();
        return filename;
    }

    void setEndNote(const std::string &note) {
        mEndNote = note;
    }

    void showEndNote() {
        if (mEndNote.empty()) {
            return;
        }

        incCurrentLine(2);
        mConsole.addString(mEndNote);
    }

    static KeyEvent getEvent(int ch) {
        switch (ch) {
        case CURSES_KEY_d:
            return KeyEvent::KEY_d;
        case CURSES_KEY_f:
            return KeyEvent::KEY_f;
        case CURSES_KEY_l:
            return KeyEvent::KEY_l;
        case CURSES_KEY_p:
            return KeyEvent::KEY_p;
        case CURSES_KEY_q:
            return KeyEvent::KEY_q;
        case CURSES_KEY_s:
            return KeyEvent::KEY_s;
        default:
            return KeyEvent::UNRECOGNIZED_KEY;
        }
    }

private:
    void incCurrentLine(int inc = 1) {
        mCurrentLine += inc;
        mConsole.moveCursor(0, mCurrentLine);
    }

private:
    CursesConsole &mConsole;
    AudioPlayer &mAudioPlayer;

    int mCurrentLine = 0;
    std::string mEndNote;
};

#endif // AUDIO_PLAYER_APP_H
