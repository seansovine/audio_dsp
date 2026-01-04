#!/usr/bin/env python

# Use SciPy to design IIR filters for graphic EQ.

import matplotlib.pyplot as plt
import matplotlib.ticker
import numpy as np

from pprint import pp
from scipy import signal


def lowpass():
    sampling_frequency = 44_100
    nyquist_rate = sampling_frequency / 2
    passband_start = 1000 / nyquist_rate
    stopband_start = 1200 / nyquist_rate
    passband_max_loss = 1
    stopband_min_atten = 40

    system = signal.iirdesign(
        wp=passband_start,
        ws=stopband_start,
        gpass=passband_max_loss,
        gstop=stopband_min_atten,
    )
    pp(system)

    return system


def plot_system(system):
    """
    Plot frequency amplitude and phase response of filter.
    Borrowed from iirdesign docs page.

    system : tuple of arrays
        b_n and a_n coefficients of designed IIR filter.
    """

    w, h = signal.freqz(*system)

    fig, ax1 = plt.subplots()
    ax1.set_title("Digital filter frequency response")
    ax1.plot(w, 20 * np.log10(abs(h)), "b")
    ax1.set_ylabel("Amplitude [dB]", color="b")
    ax1.set_xlabel("Frequency [rad/sample]")
    ax1.grid(True)
    ax1.set_ylim([-120, 20])
    ax2 = ax1.twinx()
    phase = np.unwrap(np.angle(h))
    ax2.plot(w, phase, "g")
    ax2.set_ylabel("Phase [rad]", color="g")
    ax2.grid(True)
    ax2.axis("tight")
    ax2.set_ylim([-6, 1])
    nyticks = 8
    ax1.yaxis.set_major_locator(matplotlib.ticker.LinearLocator(nyticks))
    ax2.yaxis.set_major_locator(matplotlib.ticker.LinearLocator(nyticks))
    nxticks = 24  # enough to show stopband cutoff
    ax1.xaxis.set_major_locator(matplotlib.ticker.LinearLocator(nxticks))
    plt.show()


if __name__ == "__main__":
    system = lowpass()
    plot_system(system)
