// Thread-safe queue class from Anthony Williams' C++ concurrency book.
//
// Created by sean on 1/9/25.

#ifndef THREADSAFE_QUEUE_H
#define THREADSAFE_QUEUE_H

#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>

// Copied from the code samples from Anthony Williams'
// book _C++ Concurrency in Action_, second edition.
//
// See: https://github.com/anthonywilliams/ccia_code_samples
// This is Listing 4.5.

template <typename T> class ThreadsafeQueue {
    std::queue<T> data_queue;

    mutable std::mutex mut;
    std::condition_variable data_cond;

  public:
    ThreadsafeQueue() = default;

    ThreadsafeQueue(ThreadsafeQueue const &other) {
        std::lock_guard<std::mutex> lk(other.mut);

        data_queue = other.data_queue;
    }

    void push(T &&new_value) {
        std::lock_guard lk(mut);

        data_queue.push(std::forward<T>(new_value));
        data_cond.notify_one();
    }

    void wait_and_pop(T &value) {
        std::unique_lock lk(mut);

        data_cond.wait(lk, [this] { return !data_queue.empty(); });
        value = data_queue.front();
        data_queue.pop();
    }

    std::shared_ptr<T> wait_and_pop() {
        std::unique_lock lk(mut);
        data_cond.wait(lk, [this] { return !data_queue.empty(); });

        std::shared_ptr<T> res(std::make_shared<T>(data_queue.front()));
        data_queue.pop();

        return res;
    }

    bool try_pop(T &value) {
        std::lock_guard lk(mut);

        if (data_queue.empty()) {
            return false;
        }

        value = data_queue.front();
        data_queue.pop();

        return true;
    }

    std::shared_ptr<T> try_pop() {
        std::lock_guard lk(mut);

        if (data_queue.empty()) {
            return std::shared_ptr<T>();
        }

        std::shared_ptr<T> res(std::make_shared<T>(data_queue.front()));
        data_queue.pop();

        return res;
    }

    bool empty() const {
        std::lock_guard lk(mut);

        return data_queue.empty();
    }
};

#endif // THREADSAFE_QUEUE_H
