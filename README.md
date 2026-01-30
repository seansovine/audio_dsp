# Real-time Audio and DSP

This repository contains projects and notes for learning real-time audio
programming and digital signal processing for audio applications.

## Why this project?

A few years ago I completed a PhD in math in Fourier analysis, but after graduating
I decided to go into software engineering instead of academic research. Since
then I've been putting some effort into learning hands-on signal processing. I've found that
my mathematical background has been helpful, but there is a lot more that I've had to learn
and am still learning. I also just find audio programming and signal processing very
interesting, so this project is also just for fun.

These [notes](./Details.md) contain some more technical details and background information
about the project.

## ALSA console audio player

The main project in this repository is a basic console audio player built using ALSA.
It has a simple ncurses command-line user interface that allows the user to
load a file and to start and stop playback.
For loading the file data and performing FFTs it uses
[kfr](https://github.com/kfrlib/kfr), which is a nice library for DSP in C++.

<p align="center" margin="20px">
        <img src="https://raw.githubusercontent.com/seansovine/page_images/refs/heads/main/screenshots/alsa_player/alsa_player_2026-01-05.png" alt="drawing" width="500" style="padding-top: 10px; padding-bottom: 10px"/>
</p>

The colorful arrow in the middle of the image is a real-time sound level indicator, and
the line below it is a progress bar showing how much of the file has played. The four bars
below that are levels of the frequency bins of our spectal analysis, which is described in the notes.

### Building and running

There is a CMake build system, but the Makefile is there for convenience:

```shell
# Clone repo and then get submodules.
git submodule update --init --recursive

# Configure and build.
make configure
make build

# Run it.
make run

```

There are a few development packages needed for the build; when I can build it in a
clean environment I'll make a list of them. Otherwise, the project should be self-contained.
