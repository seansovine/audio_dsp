// The start of a console audio player using ALSA and KFR.
//
// Created by sean on 1/1/25.
//

// clang-format off
#include "lib/alsa_player.h"
#include "lib/audio_player.h"

#include "root_directory.h"

#include <fmt/core.h>
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

    fmt::print("Playing audio file: {}\n", inFilename);

    auto inFile = std::make_shared<AudioFile>(inFilename);

    // --------------------------
    // Play file with AlsaPlayer.

    PlaybackState state;
    AlsaPlayer player{state};

    player.init(inFile);
    player.printInfo();

    player.play();
    player.shutdown();

    // -----
    // Done.

    return 0;
}
