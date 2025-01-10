//
// Created by sean on 1/10/25.
//

#ifndef AUDIO_PLAYER_APP_H
#define AUDIO_PLAYER_APP_H

#include "curses_console.h"
#include "root_directory.h"

#include <fmt/color.h>

// ----------------------------------
// For managing the state of the app.

enum class State {
    Stopped,
    Playing,
};

enum class KeyEvent { KEY_q, KEY_l, UNRECOGNIZED_KEY };

struct AppState {
    State mCurrentState = State::Stopped;

    std::string mFilepath;
    std::shared_ptr<AudioFile> mAudioFile = nullptr;

    MessageQueue mQueue;
    PlaybackState mPlaybackState;
};

// Manages the underlying app state.

class AudioPlayer {
  public:
    AudioPlayer() = default;

    AppState &appState() { return mAppState; }

    bool fileIsLoaded() const { return mAppState.mAudioFile != nullptr; }

    void loadAudioFile(const std::string &filePath) {
        static const auto testFilename = std::string(project_root) + "/media/Low E.wav";

        std::string inFilename = testFilename;

        if (!filePath.empty()) {
            inFilename = filePath;
        }

        auto inFile = std::make_shared<AudioFile>(inFilename);
        mAppState.mAudioFile = std::move(inFile);
        mAppState.mFilepath = inFilename;
    }

    void handleEvent(KeyEvent event) {
        switch (event) {
        case KeyEvent::KEY_l: {
            loadAudioFile("");
            break;
        }

        default:
            break;
        }
    }

  private:
    AppState mAppState;
};

// Interface between audio player and curses console.

class ConsoleManager {
  public:
    ConsoleManager(CursesConsole &console, AudioPlayer &player)
        : mConsole(console), mAudioPlayer(player) {}

    void showFileStatus() {
        mConsole.moveCursor(0, 2);

        if (mAudioPlayer.fileIsLoaded()) {
            auto msg = fmt::format("Audio file loaded: {}", mAudioPlayer.appState().mFilepath);
            mConsole.addString(msg);
        } else {
            mConsole.addString("Audio file not loaded.");
        }
    }

    void showHeader() {
        mConsole.moveCursor(0, 0);
        mConsole.addString("> Simple ALSA Audio Player <");
    }

    void showOptions() {
        constexpr int startLine = 4;
        int currentLine = startLine;

        mConsole.moveCursor(0, currentLine);

        if (!mAudioPlayer.fileIsLoaded()) {
            mConsole.addString("Press l to load file.");
            currentLine += 1;
        }

        mConsole.moveCursor(0, currentLine);
        mConsole.addString("Press q to exit.");
    }

    static KeyEvent getEvent(int ch) {
        switch (ch) {
        case CURSES_KEY_q:
            return KeyEvent::KEY_q;
        case CURSES_KEY_l:
            return KeyEvent::KEY_l;
        default:
            return KeyEvent::UNRECOGNIZED_KEY;
        }
    }

  private:
    CursesConsole &mConsole;
    AudioPlayer &mAudioPlayer;
};

#endif //AUDIO_PLAYER_APP_H
