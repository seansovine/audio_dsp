// A class for managing the state of the console user interface.
//
// Created by sean on 7/12/25.

#ifndef CONSOLE_MANAGER_H
#define CONSOLE_MANAGER_H

#include "audio_player_app.hpp"
#include "curses_console.hpp"

#include <fmt/format.h>

using ColorPair = CursesConsole::ColorPair;

// ----------------
// Console manager.

// Interface between curses console and audio player state.
// This should read, but never modify, the audio player state.

class ConsoleManager {
  public:
    ConsoleManager(CursesConsole &console, AudioPlayer &player)
        : mConsole(console),
          mAudioPlayer(player) {
    }

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
        } else if (mAudioPlayer.currentState() == State::Stopped) {
            incCurrentLine(1);
        }
        incCurrentLine(1);
    }

    void debugState(int lineNum) {
        int currentLine = mCurrentLine;
        mConsole.moveCursor(0, lineNum);
        mConsole.addStringWithColor(stateString(mAudioPlayer.currentState()),
                                    ColorPair::RedOnBlack);
        mCurrentLine = currentLine;
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
        default: {
            throw std::logic_error("Invalid state in showOptions.");
        }
        }

        mConsole.addString("Press q to exit.");
    }

    std::string getFilename() {
        incCurrentLine(2);
        mConsole.addString("Enter filename to load: ");

        return mConsole.getString();
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

    void addLine(const std::string &line) {
        mConsole.addString(line);
        incCurrentLine(1);
    }

    void clearLine() {
        int screenWidth = mConsole.getScreenSize().second;
        mConsole.moveCursor(0, mCurrentLine);
        mConsole.addString(std::string(screenWidth, ' '));
    }

    void showSoundLevel(float intensity) {
        int intensityLevel = 1 + static_cast<int>(std::round(intensity));
        int greenParts = std::min(intensityLevel - 1, 15);
        int yellowParts = std::max(0, intensityLevel - (greenParts + 1));
        clearLine();

        // draw current state of sound level meter
        mConsole.moveCursor(0, mCurrentLine);
        mConsole.addChar(']');
        mConsole.addStringWithColor(std::string(greenParts, '='), ColorPair::GreenOnBlack);
        mConsole.addStringWithColor(std::string(yellowParts, '='), ColorPair::YellowOnBlack);
        mConsole.redOnBlack();
        mConsole.addChar('>');
        mConsole.whiteOnBlack();
        incCurrentLine(2);
    }

    void showTimeBar(float propDone) {
        constexpr int BAR_WIDTH = 41;

        mConsole.addChar('[');
        for (int i = 0; i < BAR_WIDTH; i++) {
            int current = BAR_WIDTH * propDone;
            if (i == current) {
                mConsole.addChar('+');
            } else if (i == BAR_WIDTH / 2) {
                mConsole.addChar('|');
            } else {
                mConsole.addChar('-');
            }
        }

        mConsole.addChar(']');
        incCurrentLine(2);
    }

    static KeyEvent getEvent(int ch) {
        switch (ch) {
        case CURSES_KEY_d: {
            return KeyEvent::KEY_d;
        }
        case CURSES_KEY_f: {
            return KeyEvent::KEY_f;
        }
        case CURSES_KEY_l: {
            return KeyEvent::KEY_l;
        }
        case CURSES_KEY_p: {
            return KeyEvent::KEY_p;
        }
        case CURSES_KEY_q: {
            return KeyEvent::KEY_q;
        }
        case CURSES_KEY_s: {
            return KeyEvent::KEY_s;
        }
        default: {
            return KeyEvent::UNRECOGNIZED_KEY;
        }
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

#endif // CONSOLE_MANAGER_H
