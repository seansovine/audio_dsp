// A first test program using the Q library.
// It simply reads
//
// Created by sean on 12/17/24.
//

#include <q_io/audio_file.hpp>

#include <iostream>
#include <ostream>

namespace q = cycfi::q;

// TODO: Get rid of the absolute path here.
// (Have CMake generate a header with project dir constant.)
static const std::string media_dir = "/home/sean/Code/A-K/audio_dsp/src/examples/media/";
static const std::string output_dir = "/home/sean/Code/A-K/audio_dsp/src/examples/output/";

int main() {
    q::wav_reader wav_in{media_dir + "Low E.wav"};

    if (wav_in) {
        std::vector<float> in(wav_in.length());
        wav_in.read(in);

        // TODO: Do stuff with WAV data.
        // ...

        // Reduce signal amplitude, to test.
        for (float& val : in) {
            val *= 0.1;
        }

        q::wav_writer wav_out(output_dir + "Low E.wav", wav_in.num_channels(), wav_in.sps());
        wav_out.write(in);
    } else {
        std::cerr << "Wav file not found." << std::endl;
    }

    return 0;
}
