#ifndef PROCESSING_THREAD_H_
#define PROCESSING_THREAD_H_

#include "alsa_player.hpp"
#include "rt_queue.hpp"
#include <dsp/dsp_tools.hpp>

#include <kfr/dft/fft.hpp>

#include <cstdlib>
#include <array>
#include <atomic>
#include <cassert>
#include <complex>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <numbers>

namespace proc_thread {

// Range of frequencies to display.
static constexpr size_t MIN_FREQ = 60;
static constexpr size_t MAX_FREQ = 12'000;

// Binning config.
static constexpr size_t NUM_SPECTROGRAM_BINS = 4;

// Octave-based distribution into bins.
static constexpr double BIN_0_CUTOFF = 250;
static constexpr double BIN_1_CUTOFF = 1000;
static constexpr double BIN_2_CUTOFF = 4000;

static size_t getBin(double freq) {
    if (freq <= BIN_0_CUTOFF) {
        return 0;
    } else if (freq <= BIN_1_CUTOFF) {
        return 1;
    } else if (freq <= BIN_2_CUTOFF) {
        return 2;
    } else {
        return 3;
    }
}

} // namespace proc_thread

using MainQueue = QueueHolder<proc_thread::NUM_SPECTROGRAM_BINS>;
using ProcQueue = QueueHolder<alsa_player::PROCESSING_WINDOW_SIZE>;

class ProcessingThread {
    // Queues are named after their receiver.
    MainQueue mMainThreadQueue;
    ProcQueue mProcessingQueue;

    // To allow external shutdown.
    std::atomic_bool &mRunning;

    uint32_t mAudioSampleRate = 0;

  public:
    ProcessingThread(MainQueue mainThreadQueue, ProcQueue processingQueue,
                     std::atomic_bool &running)
        : mMainThreadQueue(mainThreadQueue),
          mProcessingQueue(processingQueue),
          mRunning(running) {
    }

    void setAudioSampleRate(uint32_t audioSampleRate) {
        assert(!mRunning);
        mAudioSampleRate = audioSampleRate;
    }

    void operator()() {
        constexpr size_t FFT_LEN = 1.5 * alsa_player::PROCESSING_WINDOW_SIZE;
        constexpr size_t NUM_BINS = proc_thread::NUM_SPECTROGRAM_BINS;
        size_t bindWidth = (proc_thread::MAX_FREQ - proc_thread::MIN_FREQ) / NUM_BINS;

        std::array hannWindow = dsp::makeHannWindow<FFT_LEN>();
        kfr::dft_plan_real<double> plan(FFT_LEN);
        kfr::univector<std::complex<double>, FFT_LEN> prevFftData{0};

        using namespace std::complex_literals;
        using namespace std::numbers;

        // Euler's formula for e^(i*pi*2/3) for modulating previous FFT for overlap add.
        static const std::complex<double> MODULATION_FACTOR =
            std::cos(2.0 * pi / 3.0) + std::sin(2.0 * pi / 3.0) * 1i;

        while (true) {
            while (mRunning && mProcessingQueue.queueRef.size() == 0)
                ;
            // Discard older data and get the most recent.
            while (mProcessingQueue.queueRef.size() > 1) {
                mProcessingQueue.queueRef.pop();
            }
            if (!mRunning) {
                break;
            }

            alsa_player::AlsaData::array_type &windowData = mProcessingQueue.queueRef.front()->data;

            // Copy data with window function applied into second two-thirds of buffer.
            kfr::univector<double, FFT_LEN> inData = {0.0};
            for (size_t i = FFT_LEN / 3; i < FFT_LEN; i++) {
                inData[i] = hannWindow[i] * windowData[i];
            }
            mProcessingQueue.queueRef.pop();

            // Take fourier transform of windowed data.
            kfr::univector<cometa::u8> temp(plan.temp_size);
            kfr::univector<std::complex<double>, FFT_LEN> fftData;
            plan.execute(fftData, inData, temp);

            auto getFreqHerz = [FFT_LEN, this](size_t harmonic) -> double {
                size_t folded = harmonic > FFT_LEN / 2 ? FFT_LEN - harmonic : harmonic;

                // This is sample rate in Hz / period of sinusoid = oscillation frequency of
                // sinusoidal component of Fourier decomposition. In other words, this is the
                // frequency of the signal that this component represents.
                return mAudioSampleRate * (folded / (double)FFT_LEN);

                // For binning here, we count frequencies along with their negatives, because we
                // only care about the rates of oscillation, not the effects of cancellation.
                //
                // Note: Recall that the real parts of the coefficients are even, and
                //       note that the highest unique absolute frequency represented
                //       is the Nyquist frequency.
            };

            // Overlap add FFT with previous FFT data and put in appropriate frequency bins.
            MainQueue::data_type newData{};
            std::complex<double> modulation = 1;
            for (size_t harmonic = 0; harmonic < FFT_LEN; harmonic++) {
                size_t realFreq = getFreqHerz(harmonic);

                if (realFreq < proc_thread::MIN_FREQ || proc_thread::MAX_FREQ < realFreq) {
                    continue;
                }

                // We translate the previous signal back in time by half the FFT length;
                // since FT converts time translation to modulation, we multiply by a
                // modulation factor, which is a pure complex exponential.
                modulation *= MODULATION_FACTOR;
                // This is multiplying previous FFT sequence X(k) by e^(i*pi*k) = (-1)^k,
                // which is equivalent to FFT[X(k-N/2)]. Because the Fourier transform
                // is linear,this amounts to FFTing the overlap of two sampling windows.
                std::complex<double> overlapped =
                    fftData[harmonic] + modulation * prevFftData[harmonic];

                // Add magnitude of coefficient (roughly energy in this frequency)
                // of the FFT of overlapped windows to the appropriate bin.
                newData.data[proc_thread::getBin(realFreq)] += std::abs(overlapped);
            }

            // Put data in queue or drop it.
            bool _ = mMainThreadQueue.queueRef.try_push(newData);
            prevFftData = std::move(fftData);
        }
    }
};

#endif // PROCESSING_THREAD_H_
