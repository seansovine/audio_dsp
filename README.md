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

Much of this project is figuring things out as I go along based on some reading
and general knowledge of algorithms, data structures, math, and hardware. I am probably reinventing
a few wheels and making some mistakes that would seem obvious or naive to an experienced audio
programmer. But the point of the project is to learn, and this gives us the chance to do that.

And, the result does work the way it's supposed to, so that counts for something.

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
below that are the frequency bin levels of our spectal analysis, which is described below.

__Main files:__

The code that interfaces with the ALSA driver is in the files

+ [`alsa_player.h`](src/audio_player/lib/alsa_player.h)
+ [`alsa_player.cpp`](src/audio_player/lib/alsa_player.cpp)

and the top-level files defining the application and the user interface are:

+ [`audio_player_app.h`](src/audio_player/lib/audio_player_app.h)
+ [`console_manager.h`](src/audio_player/lib/console_manager.h)
+ [`audio_player_main.cpp`](src/audio_player/audio_player_main.cpp)

### Building and running

The Makefile makes it convenient:

```shell
# Clone repo and then get submodules.
git submodule update --init --recursive

# Configure and build.
make configure
make build

# Run it.
make run

```

You may need to install some packages if your environment is different than mine, which is likely,
but otherwise the project is self-contained.

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

I believe that nothing we are currently doing comes close to using up the budget of processing
between buffer writes on modern laptop CPUs. I could do some work to confirm this with numbers.
But also, I'm interested in doing things on less powerful devices like microcontrollers, and there
we will have less processor speed and power to work with.

__Real-time spectral analysis:__

The UI now has a real-time spectral analysis display. This is computed using the short-time Fourier
transform with the Hann window function, with 50% overlap between windows. This allows a balance
between accurate frequency representation and lower latency frequency updates. The magnitudes
of the Fourier coefficients obtained in this way are split into four bins using an octave-based
division of the frequency range from 60hz to 12khz. These ranges were chosen roughly based on human
sound perception, as humans tend to perceive pitch in octaves.

The spectral analysis is performed on a background processing thread, in the file

+ [`processing_thread.cpp`](src/audio_player/lib/processing_thread.hpp)

For the lock-free queue implementation it uses [SPSCQueue](https://github.com/rigtorp/SPSCQueue).

If I get around to it, I will write up some more details on the math and implementation of windowed
FFT (STFT) for spectral analysis. It was hard to find information on this specific application in
one place at a practical level. Most sources I found that describe it are either from first principles
in great detail, or mentioned it in passing among more complex applications.

__IIR filter bass boost:__

There is now an IIR filter-based bass boost effect that can be activated to modify the audio during
playback. This is implemented in the file

+ [`filter.hpp`](src/audio_player/lib/filter.hpp)

The initial version of this is pretty bare-bones, and there are a few TODOs to improve it. For example,
the spectral analysis currently only applies to the original signal, not the modified one. I will
update this so the spectrum data reflects the boosted audio. In the future I will expand this effect
into a graphic equalizer that allows the user to adjust the level of each frequency band during playback.

## More ideas for future work:

+ I plan to expand the bass boost to a graphic EQ.

+ I will add real times to the UI progress bar, and eventually pause and seek controls.

Beyond these, I plan to read much more on professional audio processing and DSP and to study the
code of some of the many good open source audio projects. There are many details of low-level
concurrency that I only have a passing knowledge of, and it would very useful to clarify those.
There are also many concurrency techniques that are used in audio and other real-time systems that
I only have a basic understanding of, and I will look into those too. This project has given hands-on
experience with some of those things, and a place to try out different versions of them.
