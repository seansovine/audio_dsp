#ifndef RT_QUEUE_H_
#define RT_QUEUE_H_

// Definitions for working with SPSCQueue.

#include <rigtorp/SPSCQueue.h>

#include <array>
#include <cstddef>

using namespace rigtorp;

static constexpr std::size_t QUEUE_CAP = 20;

template <size_t N>
struct Data {
    using array_type = std::array<float, N>;

    array_type data;
};

template <size_t N>
using DataQueue = SPSCQueue<Data<N>>;

template <size_t N>
struct QueueHolder {
    using queue_type = DataQueue<N>;
    using data_type = Data<N>;

    queue_type &queueRef;
};

#endif // RT_QUEUE_H_
