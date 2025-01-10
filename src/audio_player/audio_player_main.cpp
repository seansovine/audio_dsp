// The start of a console audio player using ALSA and KFR.
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

    while (player.running()) {
        if (player.updateState()) {
            console.clearBuffer();
        }

        manager.showHeader();
        manager.showFileStatus();
        manager.showOptions();
        manager.showEndNote();

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
    }

    // -----
    // Done.

    return 0;
}

// TODO: Convert the code below into appropriate methods
// to be called when app is in appropriate states.

// // --------------------------------------------------------
// // Play the file with basic visualization of avg intensity.
//
// // Test thread communication by reading the average intensity
// // statistic shared variable, then ending playback early.
//
// constexpr int msToRun = 7'000;
// constexpr int msPerSample = 125;
//
// for (int i = 0; i < msToRun / msPerSample; i++) {
//     float intensity = playbackState.mAvgIntensity;
//     int intensityLevel = 1 + static_cast<int>(std::round(intensity * 500));
//
//     auto bars = std::string(intensityLevel, '0');
//     fmt::print(">>: {}\n", bars);
//
//     std::this_thread::sleep_for(std::chrono::milliseconds(msPerSample));
// }
