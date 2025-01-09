// The start of a console audio player using ALSA and KFR.
//
// Created by sean on 1/1/25.
//

// clang-format off
#include "lib/alsa_player.h"
#include "lib/audio_player.h"

#include "root_directory.h"

#include <fmt/color.h>

#include <chrono>
#include <thread>
// clang-format on

// -------------
// Main program.

int main(int argc, char *argv[]) {
    // ---------------------
    // Load audio file data.

    static const auto testFilename = std::string(project_root) + "/media/Low E.wav";

    std::string inFilename = testFilename;

    if (argc > 1) {
        inFilename = argv[1];
    }

    fmt::print("Playing audio file: {}\n\n", inFilename);

    auto inFile = std::make_shared<AudioFile>(inFilename);

    // --------------------------
    // Play file with AlsaPlayer.

    MessageQueue queue;
    PlaybackState state;

    AlsaPlayer player{state, queue};

    auto playbackThreadFn = [&player, &queue](const auto &inFile) -> void {
        player.init(inFile);

        AlsaInfo info;
        player.getInfo(&info);

        // NOTE: It appears we can't use dynamic allocation inside the
        // translation unit where ALSA functions are called. When we
        // try the result is that ALSA stops working. This is possibly
        // a linking conflict or maybe some sort of allocator conflict?
        //
        // Until we figure this out we'll have to do our logging here in
        // this thread, rather than inside AlsaPlayer itself.

        Logger logger{queue};

        logger.logFmt("Number of channels: {}", info.mNumChannels);
        logger.logFmt("Sample rate: {}", info.mSampleRate);

        logger.log("");
        logger.log("Playing sound data from file...");

        player.play();
        player.shutdown();

        logger.log("Playback complete.");
    };

    std::thread playbackThread(playbackThreadFn, inFile);

    // --------------------------------------------------------
    // Play the file with basic visualization of avg intensity.

    // Test thread communication by reading the average intensity
    // statistic shared variable, then ending playback early.

    constexpr int msToRun = 7'000;
    constexpr int msPerSample = 125;

    for (int i = 0; i < msToRun / msPerSample; i++) {
        float intensity = state.mAvgIntensity;
        int intensityLevel = 1 + static_cast<int>(std::round(intensity * 500));

        auto bars = std::string(intensityLevel, '0');
        fmt::print(">>: {}\n", bars);

        std::this_thread::sleep_for(std::chrono::milliseconds(msPerSample));
    }

    fmt::print("\nStopping audio playback from UI thread.\n");
    queue.push("Stopping audio playback from UI thread.");

    state.mPlaying = false;

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
