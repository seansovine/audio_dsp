//
// Created by sean on 12/31/24.
//

#include "root_directory.h"

#include <fmt/core.h>

#include <kfr/io.hpp>

static const auto filename = std::string(project_root) + "/media/Low E.wav";

int main() {
    fmt::print("Attempting to open file: {}\n", filename);

    auto file = kfr::open_file_for_reading(filename);
    kfr::audio_reader_wav<float> reader{file};

    fmt::print("Audio file loaded successfully.\n");
}
