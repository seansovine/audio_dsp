// A program to test some of kfr's FFT features.
//
// Created by sean on 12/13/25.
//

#include <dsp/dsp_tools.hpp>
#include <root_directory.h>

#include <fmt/base.h>
#include <fmt/core.h>
#include <kfr/dsp/window.hpp>
#include <kfr/io.hpp>

#include <cmath>
#include <cstddef>

static const auto inFilename =
    std::string(project_root) + "/scratch/espressif_ff-16b-2c-44100hz.wav";

constexpr size_t HANN_SIZE = 64;

static auto HANN_WIN = makeHannWindow<HANN_SIZE>();

int main() {
    if (false) {
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
    }

    const std::string options = "freqresp=True, dots=True, padwidth=1024, "
                                "log_freq=False, horizontal=False, normalized_freq=True";
    kfr::univector<kfr::fbase, HANN_SIZE> output;

    for (size_t i = 0; i < HANN_SIZE; i++) {
        output[i] = HANN_WIN[i] + HANN_WIN[(i + HANN_SIZE / 2) % HANN_SIZE];
        fmt::println("Overlapped Hann sample {:2}: {}", i, output[i]);
    }

    // output = kfr::window_hann(output.size());

    fmt::println("Num samples in Hann window = {}", output.size());

    // Test overlap property.
    // for (size_t i = output.size() / 2; i < output.size(); i++) {
    //     output[i] += output[(i + output.size() / 2) % output.size()];
    //     fmt::println("Sample {:2}: {}", i, output[i]);
    // }

    plot_save("window_hann", output, options + ", title='Hann window'");
}
