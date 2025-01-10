// The start of a console audio player using ALSA and KFR.
//
// Created by sean on 1/1/25.
//

// clang-format off
#include "lib/alsa_player.h"
#include "lib/audio_player.h"
#include "lib/audio_player_app.h"
#include "curses_console.h"

#include <fmt/color.h>

#include <chrono>
#include <thread>
// clang-format on

// -------------------------
// Audio playback thread fn.

class PlayBackThread {
  public:
    explicit PlayBackThread(AppState &appState)
        : mLogger(Logger{appState.mQueue}), mPlaybackState(appState.mPlaybackState),
          mAudioFile(appState.mAudioFile) {}

    void run() {
        mLogger.log("Playing sound data from file...");

        AlsaPlayer player{mPlaybackState};
        player.init(mAudioFile);

        AlsaInfo info;
        player.getInfo(&info);

        player.play();
        player.shutdown();

        mLogger.log("Playback complete.");
    }

  private:
    Logger mLogger;
    PlaybackState &mPlaybackState;

    std::shared_ptr<AudioFile> mAudioFile;
};

// -------------
// Main program.

int main(int argc, char *argv[]) {
    AudioPlayer player;
    AppState &appState = player.appState();

    // Ncurses console interface.
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

    while (true) {
        manager.showHeader();
        manager.showFileStatus();
        manager.showOptions();

        if (int ch = console.getChar(); ch != CursesConsole::NO_KEY) {
            if (ch == CURSES_KEY_q) {
                break;
            }
            player.handleEvent(ConsoleManager::getEvent(ch));
            console.clearBuffer();
        }
    }

    return 0;

    // TODO: Convert the code below into appropriate methods
    // to be called when app is in appropriate states.

    // --------------------------
    // Play file with AlsaPlayer.

    std::thread playbackThread([&appState]() {
        PlayBackThread pbThread{appState};
        pbThread.run();
    });

    // References for convenience.
    MessageQueue &queue = appState.mQueue;
    PlaybackState &playbackState = appState.mPlaybackState;

    // --------------------------------------------------------
    // Play the file with basic visualization of avg intensity.

    // Test thread communication by reading the average intensity
    // statistic shared variable, then ending playback early.

    constexpr int msToRun = 7'000;
    constexpr int msPerSample = 125;

    for (int i = 0; i < msToRun / msPerSample; i++) {
        float intensity = playbackState.mAvgIntensity;
        int intensityLevel = 1 + static_cast<int>(std::round(intensity * 500));

        auto bars = std::string(intensityLevel, '0');
        fmt::print(">>: {}\n", bars);

        std::this_thread::sleep_for(std::chrono::milliseconds(msPerSample));
    }

    fmt::print("\nStopping audio playback from UI thread.\n");
    queue.push("Stopping audio playback from UI thread.");

    playbackState.mPlaying = false;

    // Wait on playback thread to complete.

    playbackThread.join();

    // -----------------------------------------
    // Get all messages the player put in queue.

    // TODO: When we add a curses user interface, we will get these
    // messages from the queue as they arrive in the main UI loop.
    // For now this is just here to test that messages are received.

    fmt::print(fmt::emphasis::bold | fmt::fg(fmt::color::green), "\nEmptying message queue:\n\n");

    while (!queue.empty()) {
        fmt::print("{}\n", *queue.try_pop());
    }

    // -----
    // Done.

    return 0;
}
