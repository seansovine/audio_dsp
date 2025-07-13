// A console audio player built using ALSA.
//
// Created by sean on 1/1/25.

#include "lib/audio_player_app.h"
#include "lib/console_manager.h"

// -------------
// Main program.

int main() {
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

    // Set timeout for getchar.
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
        if (player.currentState() == State::Playing) {
            if (subsampleCounter % subsampleRate == 0) {
                intensitySample = player.appState().mPlaybackState.mAvgIntensity;
            }
            manager.showSoundLevel(intensitySample);
        } else if (player.currentState() == State::Stopped) {
            manager.showSoundLevel(0.0f);
        }

        manager.showOptions();
        manager.showEndNote();

        // TODO: For debugging use.
        // manager.debugState(12);

        // Handle user input.
        if (int ch = console.getChar(); ch != CursesConsole::NO_KEY) {
            State state = player.handleEvent(ConsoleManager::getEvent(ch));

            if (state == State::FilenameInput) {
                // NOTE: Want to use `manager.getFilename()` here, but doing so
                // interferes with the sound level meter display (but not playback!).
                // There must be some undefined behavior or global state being set in
                // that call, though clang sanitizer doesn't see UB in code we compile.
                //
                // TODO: Investigate further.
                std::string filename = "media/Low E.wav";

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
