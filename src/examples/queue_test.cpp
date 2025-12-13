// Test program for the lock-free queue library.

#include <rigtorp/SPSCQueue.h>

#include <array>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string>
#include <thread>

using namespace rigtorp;

struct Data {
    static constexpr std::size_t NUM_DATA_SAMPLES = 4096;
    std::array<uint8_t, NUM_DATA_SAMPLES> data;
};

struct QueueHolder {
    SPSCQueue<Data> &queueRef;
};

static constexpr std::size_t QUEUE_CAP = 20;

int main() {
    std::cout << "Test!" << std::endl;

    SPSCQueue<Data> mainThreadRx{QUEUE_CAP};
    QueueHolder mainThrdRxHolder{mainThreadRx};

    std::atomic_bool running = true;

    SPSCQueue<Data> procThreadRx{QUEUE_CAP};
    QueueHolder procThrdRxHolder{procThreadRx};

    // Processes data sent by producer thread.
    std::thread processor([mainThrdRxHolder, procThrdRxHolder, &running]() {
        while (true) {
            while (running && !procThrdRxHolder.queueRef.front())
                ;

            if (!running) {
                break;
            }

            Data rxData = *procThrdRxHolder.queueRef.front();
            procThrdRxHolder.queueRef.pop();
            rxData.data[0] *= 2;
            mainThrdRxHolder.queueRef.push(rxData);
        }
    });

    // Generates data and sends to processor thread.
    std::thread producer([procThrdRxHolder]() {
        for (uint32_t i = 0; i < 10; i++) {
            Data newData{};
            newData.data[0] = i;

            // If queue is full, drop data and move on.
            bool _ = procThrdRxHolder.queueRef.try_push(newData);

            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    });

    uint32_t rxed = 0;
    while (rxed < 10) {
        while (!mainThreadRx.front())
            ;
        std::cout << std::to_string(mainThreadRx.front()->data[0]) << std::endl;
        mainThreadRx.pop();
        rxed++;
    }

    std::cout << "Done!" << std::endl;
    running = false;
    processor.join();
    std::cout << "Processor thread shut down!" << std::endl;

    producer.join();
    std::cout << "Producer thread shut down!" << std::endl;

    return 0;
}
