// Utility classes for our ALSA audio player application.
//
// Created by sean on 1/9/25.

#ifndef AUDIO_PLAYER_H
#define AUDIO_PLAYER_H

#include "threadsafe_queue.hpp"

#include <kfr/io.hpp>

#include <format>
#include <functional>

// -----------------------------------------
// Utility to make sure a function is called
// on scope exit, like Go's 'defer' keyword.

class Defer {
    using Callback = std::function<void()>;

  public:
    explicit Defer(Callback &&inFunc) {
        func = inFunc;
    }

    ~Defer() {
        func();
    }

  private:
    Callback func;
};

// -------------------------------------------------
// Provides interface to audio file loaded with kfr.

class AudioFile {
  public:
    explicit AudioFile(const std::string &path) {
        auto file = kfr::open_file_for_reading(path);

        if (file == nullptr) {
            throw std::runtime_error("Failed to open file.");
        }

        kfr::audio_reader_wav<float> reader{file};
        mFormat = reader.format();

        // For now we just read all samples at once.
        mData = reader.read(mFormat.length);
    }

    [[nodiscard]] unsigned int sampleRate() const {
        double floatRate = mFormat.samplerate;
        long intRate = static_cast<long>(floatRate);

        if (floatRate - static_cast<double>(intRate) != 0) {
            throw std::runtime_error("Sample rate read from file has fractional part.");
        }

        return intRate;
    }

    [[nodiscard]] unsigned int channels() const {
        return mFormat.channels;
    }

    const float *data() const {
        return mData.data();
    }

    [[nodiscard]] std::size_t dataLength() const {
        return mData.size();
    }

  private:
    kfr::univector<float> mData;
    kfr::audio_format_and_length mFormat;
};

// ----------------------------------
// Class to handle logging by putting
// messages into a thread-safe queue.

using MessageQueue = ThreadsafeQueue<std::string>;

class Logger {
  public:
    explicit Logger(MessageQueue &queue)
        : mQueue{queue} {
    }

    void log(std::string &&message) {
        mQueue.push(std::forward<std::string>(message));
    }

    // Wraps C++20 std::format utils and provides
    // an interface compatible with that of libfmt.
    template <typename... Args>
    void logFmt(std::string_view fmt, Args &&...args) {
        log(std::vformat(fmt, std::make_format_args(args...)));
    }

  private:
    MessageQueue &mQueue;
};

#endif // AUDIO_PLAYER_H
