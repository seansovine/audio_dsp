# Audio Digital Signal Processing

This project will contain examples and notes on
digital signal processing for audio.

We're currently using the Q library
([GitHub](https://github.com/seansovine/audio_dsp))
of Joel de Guzman.

## First example

This is in [`first_example_w_q.cpp`](src/examples/first_example_w_q.cpp).

It reads in a WAV file and extracts its sample data
using utilities from Q, then modifies the samples, simply
by reducing the amplitude to 10% of original, then writes
the modified samples out to a new WAV file.

The purpose of this is just to test that we can get access
to the sample data and successfully modify it. Now that
we've gotten this far, we can start working on more interesting
processing and learn more of the features of Q.

## Simple ALSA WAV player

A simple program to play a WAV file directly using the ALSA PCM API:

+ [`simple_alsa_audio.cpp`](src/examples/simple_alsa_audio.cpp)

This is based mostly on Alessandro Ghedini's example here:
[GitHub gist](https://gist.github.com/ghedo/963382/815c98d1ba0eda1b486eb9d80d9a91a81d995283)

The usage is like:

```shell
<BIN_DIR>/SimpleAlsaPlayer < media/<FILENAME>.wav
```

I added some comments in the code to explain the meanings
of various quantities and function calls. I also added a little
C++ class that takes a lambda in its constructor and mimics
the behavior of Go's `defer` keyword, which I think is neat :)

Some more good sources of information on ALSA are:

+ Alex Via [blog post](https://alexvia.com/post/003_alsa_playback/) on ALSA playback
+ ALSA PCM [docs](https://www.alsa-project.org/alsa-doc/alsa-lib/pcm.html)

## Next Ideas

**Graphic EQ:**

We will implement a basic graphic equalizer using
an IIR filter, and make a program that applies this filter
to an input WAV file.

It would be nice to apply the our filter in real time,
so parameters can be adjusted and the results heard as the file is playing.
For this we can use our simple audio player above, but we will also
look at some libraries and APIs that encapsulate the hardware interaction.
