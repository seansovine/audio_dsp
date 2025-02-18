// A console audio player using ALSA and KFR.
//
// Created by sean on 1/1/25.
//

// clang-format off
#include "lib/audio_player_app.h"
#include "curses_console.h"
// clang-format on

// -------------
// Main program.

int main(int argc, char *argv[]) {
    AudioPlayer player;
    CursesConsole console;

    ConsoleManager manager{console, player};

    // ------------------------
    // Setup console interface.

    // Turn off input buffering.
    console.noInputBuffer();

    // Hide cursor.
    console.cursorVisible(false);
    // Clear screen.
    console.writeBuffer();

    constexpr int TIMEOUT_MS = 50;
    console.blockingGetCh(TIMEOUT_MS);

    // ----------------------------
    // Console interface main loop.

    // We sample intensity less often to avoid contention
    // with the real-time playback thread for atomics.
    constexpr unsigned int subsampleRate = 2;
    unsigned int subsampleCounter = 0;
    float intensitySample = 0.0f;

    while (player.running()) {
        // Update state based on asynchronous tasks.
        if (player.updateState()) {
            console.clearBuffer();
        }

        manager.showHeader();
        manager.showFileStatus();

        // Display sound level if playing.
        if (player.getState() == State::Playing) {
            if (subsampleCounter % subsampleRate == 0) {
                intensitySample = player.appState().mPlaybackState.mAvgIntensity;
            }
            manager.showSoundLevel(intensitySample);
        } else if (player.getState() == State::Stopped) {
            manager.showSoundLevel(0.0f);
        }

        manager.showOptions();
        manager.showEndNote();

        // Handle user input.
        if (int ch = console.getChar(); ch != CursesConsole::NO_KEY) {
            State state = player.handleEvent(ConsoleManager::getEvent(ch));

            if (state == State::FilenameInput) {
                std::string filename = manager.getFilename();
                player.loadUserAudioFile(filename, [&manager](bool success) {
                    if (success) {
                        manager.setEndNote("File loaded successfully.");
                    } else {
                        manager.setEndNote("Failed to load file.");
                    }
                });
            } else {
                manager.setEndNote("");
            }

            console.clearBuffer();
        }

        subsampleCounter = (subsampleCounter + 1) % subsampleRate;
    }

    // -----
    // Done.

    return 0;
}
