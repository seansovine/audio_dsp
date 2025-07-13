# Developer Notes

This file contains some ideas and comments on other programs in
subfolders of this repository.

## Official ALSA PCM example

This is in: `src/examples/alsa_official/pcm.c`.

It was copied from the ALSA PCM online docs and modified by running
clang-format and making small changes suggested by clang-tidy.

## Simple ALSA playback example

A simple program to play a WAV file directly using the ALSA PCM API:

- [`simple_alsa_audio.cpp`](src/examples/simple_alsa_audio.cpp)

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

- Alex Via [blog post](https://alexvia.com/post/003_alsa_playback/) on ALSA playback
- ALSA PCM [docs](https://www.alsa-project.org/alsa-doc/alsa-lib/pcm.html)

## Q example

This is a small example using the Q library
([GitHub](https://github.com/seansovine/audio_dsp))
of Joel de Guzman.

This is in [`first_example_w_q.cpp`](src/examples/first_example_w_q.cpp).

It reads in a WAV file and extracts its sample data
using utilities from Q, then modifies the samples, simply
by reducing the amplitude to 10% of original, then writes
the modified samples out to a new WAV file.

The purpose of this is just to test that we can get access
to the sample data and successfully modify it. Now that
we've gotten this far, we can start working on more interesting
processing and learn more of the features of Q.

## Next Ideas

**Graphic EQ:**

We will implement a basic graphic equalizer using
an IIR filter, and add this to our ALSA audio player application.
We plan to apply this filter in real time,
so parameters can be adjusted and the results heard as the file is playing.

**Hardware interaction libraries:**

We are currently using ALSA, which is pretty low-level. However, there are
some good libraries providing APIs that encapsulate the hardware interaction.
We will look at some of these later.
