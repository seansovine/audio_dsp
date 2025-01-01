// A simple program to test kfr's audio file io api.
//
// Created by sean on 12/31/24.
//

#include "root_directory.h"

#include <fmt/core.h>

#include <kfr/io.hpp>

static const auto inFilename = std::string(project_root) + "/media/Low E.wav";
static const auto outFilename = std::string(project_root) + "/media/output/kfr_rewrite.wav";

int main() {
    fmt::print("Attempting to open file: {}\n", inFilename);

    // Read the file with kfr.

    auto file = kfr::open_file_for_reading(inFilename);
    kfr::audio_reader_wav<float> reader{file};

    fmt::print("Audio file loaded successfully.\n\n");

    // Output some file information from the reader.

    fmt::print("Sample Rate  = {}\n", reader.format().samplerate);
    fmt::print("Channels     = {}\n", reader.format().channels);
    fmt::print("Length       = {}\n", reader.format().length);
    fmt::print("Duration (s) = {}\n", reader.format().length / reader.format().samplerate);
    fmt::print("Bit depth    = {}\n", audio_sample_bit_depth(reader.format().type));

    // Re-write the file with kfr to make sure we can still play it.

    auto outFile = kfr::open_file_for_writing(outFilename);

    // Write with same format it was originally written with.
    auto outFormat = kfr::audio_format{reader.format().channels, reader.format().type, reader.format().samplerate};
    kfr::audio_writer_wav<float> writer(outFile, outFormat);

    // This created an empty wav file, w/ only headers.
    // We now need to write the audio data to the file.

    // Read all the file's samples into a vector of floats.
    kfr::univector<float> data = reader.read(reader.format().length);

    writer.write(data.data(), data.size());
    writer.close();

    fmt::print("\nRe-wrote audio data to file: {}\n", outFilename);
}
