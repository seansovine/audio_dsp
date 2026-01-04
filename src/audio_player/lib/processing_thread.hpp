#ifndef PROCESSING_THREAD_H_
#define PROCESSING_THREAD_H_

#include "alsa_player.hpp"
#include "rt_queue.hpp"
#include <dsp/dsp_tools.hpp>

#include <kfr/dft/fft.hpp>

#include <array>
#include <atomic>
#include <cassert>
#include <complex>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace proc_thread {

// Range of frequencies to display.
static constexpr size_t MIN_FREQ = 60;
static constexpr size_t MAX_FREQ = 12'000;

// Binning config.
static constexpr size_t NUM_SPECTROGRAM_BINS = 4;

// Octave-based distribution into bins.
static constexpr size_t BIN_0_CUTOFF = 250;
static constexpr size_t BIN_1_CUTOFF = 1000;
static constexpr size_t BIN_2_CUTOFF = 4000;

static size_t getBin(size_t freq) {
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
        constexpr size_t NUM_FREQS = alsa_player::PROCESSING_WINDOW_SIZE;
        constexpr size_t NUM_BINS = proc_thread::NUM_SPECTROGRAM_BINS;

        std::array hannWindow = dsp::makeHannWindow<NUM_FREQS>();
        kfr::dft_plan_real<double> plan(NUM_FREQS);
        kfr::univector<std::complex<double>, NUM_FREQS> prevFftData{0};

        size_t bindWidth = (proc_thread::MAX_FREQ - proc_thread::MIN_FREQ) / NUM_BINS;

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

            // Copy data with window function applied.
            kfr::univector<double, NUM_FREQS> inData;
            for (size_t i = 0; i < NUM_FREQS; i++) {
                inData[i] = hannWindow[i] * windowData[i];
            }
            mProcessingQueue.queueRef.pop();

            // Take fourier transform of windowed data.
            kfr::univector<cometa::u8> temp(plan.temp_size);
            kfr::univector<std::complex<double>, NUM_FREQS> fftData;
            plan.execute(fftData, inData, temp);

            auto getFreqHerz = [NUM_FREQS, this](size_t harmonic) -> size_t {
                return (harmonic > NUM_FREQS / 2 ? NUM_FREQS - harmonic : harmonic) *
                       mAudioSampleRate / NUM_FREQS;
                // For binning, we count frequencies along with their negatives.
                //
                // Note: Recall that the real parts of the coefficients are even, and
                //       note that the highest unique absolute frequency represented
                //       is the Nyquist frequency.
            };

            // Copy data to bin.
            MainQueue::data_type newData{};
            for (size_t harmonic = 0; harmonic < NUM_FREQS; harmonic++) {
                size_t realFreq = getFreqHerz(harmonic);

                if (realFreq < proc_thread::MIN_FREQ || proc_thread::MAX_FREQ < realFreq) {
                    continue;
                }

                // Use l1-norm for efficieny; maybe not necessary.
                newData.data[proc_thread::getBin(realFreq)] +=
                    std::abs(fftData[harmonic].real() + prevFftData[harmonic].real()) +
                    std::abs(fftData[harmonic].imag() + prevFftData[harmonic].imag());
            }

            // Put data in queue or drop it.
            bool _ = mMainThreadQueue.queueRef.try_push(newData);
            prevFftData = std::move(fftData);
        }
    }
};

#endif // PROCESSING_THREAD_H_
