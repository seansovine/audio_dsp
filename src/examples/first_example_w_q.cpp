// A first test program using the Q library.
//
// It simply reads the sample data from a WAV
// file using Q's utilities, reduces the sample
// amplitude, then writes the data to a new WAV
// file.
//
// Created by sean on 12/17/24.
//

#include <q_io/audio_file.hpp>

#include <iostream>
#include <ostream>

namespace q = cycfi::q;

// TODO: Get rid of the absolute paths here.
// (Have CMake generate a header with project dir constant.)
static const std::string media_dir = "/home/sean/Code/A-K/audio_dsp/media/";
static const std::string output_dir = "/home/sean/Code/A-K/audio_dsp/media/output/";

int main() {
    q::wav_reader wav_in{media_dir + "Low E.wav"};

    if (wav_in) {
        std::vector<float> in(wav_in.length());
        wav_in.read(in);

        // Reduce signal amplitude, as a first test.
        for (auto &val : in) {
            val *= 0.1;
        }

        q::wav_writer wav_out(output_dir + "Low E.wav", wav_in.num_channels(), wav_in.sps());
        wav_out.write(in);
    } else {
        std::cerr << "Wav file not found." << std::endl;
    }

    return 0;
}
