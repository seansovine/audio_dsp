#!/usr/bin/env bash

# Build and run the audio player, for testing convenience.

set -e

build_type="Release"

/usr/local/bin/cmake --build /home/sean/Code_projects/audio_dsp/build/$build_type --config $build_type --target all "-j 8" --

build/"$build_type"/AudioPlayer
