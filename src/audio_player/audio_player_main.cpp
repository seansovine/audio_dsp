// A console audio player built using ALSA.
//
// Created by sean on 1/1/25.

#include "lib/audio_player_app.h"
#include "lib/console_manager.h"

#include <fmt/format.h>

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
                intensitySample =
                    std::max(0.0f, 0.0f + player.appState().mPlaybackState.mAvgIntensity);
            }
            manager.showSoundLevel(intensitySample);
        } else if (player.currentState() == State::Stopped) {
            manager.showSoundLevel(0.0f);
        }

        manager.showOptions();
        manager.showEndNote();

        // Handle user input.
        if (int ch = console.getChar(); ch != CursesConsole::NO_KEY) {
            State state = player.handleEvent(ConsoleManager::getEvent(ch));

            if (state == State::FilenameInput) {
                std::string filename = manager.getFilename();

                player.loadUserAudioFile(
                    filename, [&manager](bool success, std::optional<unsigned int> channels) {
                        if (success) {
                            manager.setEndNote(fmt::format(
                                "File loaded successfully.\nChannels: {:d}", channels.value()));
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
