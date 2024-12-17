# Audio Digital Signal Processing

This project will contain examples and notes on
digital signal processing for audio.

We're currently using the Q library
([GitHub](https://github.com/seansovine/audio_dsp))
of Joel de Guzman.

## First example

This is in `src/examples/first_example_w_q.cpp`.

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
an IIR filter, and make a program that applies this filter
to an input WAV file.

Eventually, it would be nice
to apply the filter in real time, so its parameters can be adjusted
as the file is playing. For that we'll need to learn something
about manipulating audio output on Linux -- probably using a library
at first.
