# Some implementation details and background

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

When get around to it, I will write up some more details on the math and implementation of windowed
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
