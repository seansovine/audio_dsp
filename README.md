# Real-time Audio and DSP

This repository contains projects and notes for real-time audio
programming and digital signal processing for audio applications.

__A bit about me:__

I have a math PhD in theoretical Fourier analysis, but after graduating I decided to go into
practical (mostly software) engineering work instead of academia. Does knowing some math help
with anything? Maybe a bit, but I still have a lot to learn. This project is part of my
efforts to turn my theoretical knowledge into hands-on signal processing experience,
and to pick up a lot of new knowledge along the way.
But most of all, I find this stuff interesting and it's just for fun.

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

__IIR filter bass boost:__

There is now an IIR filter-based bass boost effect that can be activated to modify the audio during
playback. This is implemented in the file

+ [`filter.hpp`](src/audio_player/lib/filter.hpp)

The initial version of this is pretty bare-bones, and there are a few TODOs to improve it. For example,
the spectral analysis currently only applies to the original signal, not the modified one. I will
update this so the spectrum data reflects the boosted audio. In the future I will expand this effect
into a graphic equalizer that allows the user to adjust the level of each frequency band during playback.

### More ideas for future work:

+ I plan to expand the bass boost to a graphic EQ.

+ I will add real times to the UI progress bar, and eventually pause and seek controls.
