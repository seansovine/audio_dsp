# Real-time Audio and DSP

This repository contains projects and notes for real-time audio
programming and digital signal processing for audio applications.

## ALSA console audio player

The main project in this repository is a basic console audio player built using ALSA.
It has a simple ncurses command-line user interface that allows the user to
load a file and to start and stop playback.
For loading the file data and performing FFTs it uses
[kfr](https://github.com/kfrlib/kfr), which is a nice library for DSP in C++.

<p align="center" margin="20px">
        <img src="https://raw.githubusercontent.com/seansovine/page_images/refs/heads/main/screenshots/alsa_player/alsa_player_2025-12-13.png" alt="drawing" width="500" style="padding-top: 10px; padding-bottom: 10px"/>
</p>

The colorful arrow in the middle of the image is a real-time sound level indicator, and
the line below it is a progress bar showing how much of the file has played.

__Main files:__

The code that interfaces with the ALSA driver is in the files

+ [`alsa_player.h`](src/audio_player/lib/alsa_player.h)
+ [`alsa_player.cpp`](src/audio_player/lib/alsa_player.cpp)

and the top-level files defining the application and the user interface are:

+ [`audio_player_app.h`](src/audio_player/lib/audio_player_app.h)
+ [`console_manager.h`](src/audio_player/lib/console_manager.h)
+ [`audio_player_main.cpp`](src/audio_player/audio_player_main.cpp)

### Real-time considerations:

One of the main purposes for starting this project was to do some hands-on real-time programming
with low-level audio APIs. Part of this is communication between a thread that plays the audio
and another thread that displays and manages the user interface.
To allow the user to control the audio output and for visualizing
audio information (like intensity and spectrum) as the audio is playing, we need
real-time interaction between the user interface and the processing and playback loop threads,
as the playback thread is processing samples and feeding them to the sound hardware.

We implement the real-time communication between threads using shared atomic variables
and lock-free queues, since we can't risk operations that might block the playback thread.
We also have to consider the cost of operations done in the playback loop and manage buffer
sizes in various places to strike a balance between latency and processing time and efficiency.

__Real-time spectral analysis:__

The UI now has a real-time spectral analysis display. This is computed using the short-time Fourier
transform with the Hann window function, with 50% overlap between windows. This allows a balance
between accurate frequency representation and lower latency frequency updates. The magnitudes
of the Fourier coefficients obtained in this way are sorted into four bins using an octave-based
division of the frequency range from 60hz to 12khz. These ranges were chosen roughly based on human
sound perception, as humans tend to perceive pitch in octaves.

The spectral analysis is performed on a background processing thread, in the file

+ [`processing_thread.cpp`](src/audio_player/lib/processing_thread.hpp)

For the lock-free queue implementation it uses [SPSCQueue](https://github.com/rigtorp/SPSCQueue).

### More ideas for future work:

+ I would like to implement some more sophisticated real-time signal processing
on the audio samples during playback, now that the basic framework is in place. The first thing I will probably
work on is a graphic EQ, with coefficients that the user can adjust from the interface
while the audio is playing.

+ I will add real times to the UI progress bar, and eventually pause and seek controls.
