#ifndef PROCESSING_THREAD_H_
#define PROCESSING_THREAD_H_

#include "alsa_player.hpp"
#include "rt_queue.hpp"

#include <atomic>
#include <cstddef>
#include <iostream>
#include <string>

namespace proc_thread {

static constexpr size_t NUM_SPECTROGRAM_BINS = 4;

}

using MainQueue = QueueHolder<proc_thread::NUM_SPECTROGRAM_BINS>;
using ProcQueue = QueueHolder<alsa_player::PROCESSING_WINDOW_SIZE>;

class ProcessingThread {
    // Queues are named after their receiver.
    MainQueue mMainThreadQueue;
    ProcQueue mProcessingQueue;

    // To allow external shutdown.
    std::atomic_bool &mRunning;

  public:
    ProcessingThread(MainQueue mainThreadQueue, ProcQueue processingQueue,
                     std::atomic_bool &running)
        : mMainThreadQueue(mainThreadQueue),
          mProcessingQueue(processingQueue),
          mRunning(running) {
    }

    void operator()() {
        // For initial testing just consume and discard any received data.
        std::cerr << "Processing thread startup." << std::endl;
        size_t windowsRxed = 0;
        while (true) {
            while (mRunning && !mProcessingQueue.queueRef.front())
                ;
            if (!mRunning) {
                break;
            }

            mProcessingQueue.queueRef.pop();
            std::cerr << "Received data sample " << std::to_string(windowsRxed++) << std::endl;
        }
        std::cerr << "Processing thread shutdown." << std::endl;

        // TODO: Compute STFT and put appropriate data in main thread queue.
    }
};

#endif // PROCESSING_THREAD_H_
