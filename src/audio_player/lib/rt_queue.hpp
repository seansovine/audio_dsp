#ifndef RT_QUEUE_H_
#define RT_QUEUE_H_

// Definitions for working with SPSCQueue.

#include <rigtorp/SPSCQueue.h>

#include <array>
#include <cstddef>
#include <cstdint>

using namespace rigtorp;

static constexpr std::size_t QUEUE_CAP = 20;

template <size_t N>
struct Data {
    using Array = std::array<uint8_t, N>;
    Array data;
};

template <size_t N>
using DataQueue = SPSCQueue<Data<N>>;

template <size_t N>
struct QueueHolder {
    DataQueue<N> &queueRef;
};

#endif // RT_QUEUE_H_
