//
// Created by sean on 1/1/25.
//

#include "root_directory.h"

#include <alsa/asoundlib.h>
#include <fmt/core.h>
#include <kfr/io.hpp>

#include <functional>

constexpr auto PCM_DEVICE = "default";

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

    float *data() {
        return mData.data();
    }

    [[nodiscard]] std::size_t dataLength() const {
        return mData.size();
    }

private:
    kfr::univector<float> mData;
    kfr::audio_format_and_length mFormat;
};

// -------------
// Main program.

int main(int argc, char* argv[]) {
    // ---------------------
    // Load audio file data.

    static const auto testFilename = std::string(project_root) + "/media/Low E.wav";

    std::string inFilename = testFilename;

    if (argc > 1) {
        inFilename = argv[1];
    }

    fmt::print("Playing audio file: {}\n", inFilename);

    AudioFile inFile(inFilename);

    unsigned int channels = inFile.channels();
    unsigned int rate = inFile.sampleRate();

    // ---------------
    // Setup ALSA PCM.

    fmt::print("Preparing ALSA...\n\n");

    int pcmResult = 0;
    snd_pcm_t *pcm_handle;

    // Try opening the device.
    //
    // NOTE: Mode 0 is the default BLOCKING mode.
    // So our calls to snd_pcm_writei below will block
    // until all frames sent are played or buffered.

    pcmResult = snd_pcm_open(&pcm_handle, PCM_DEVICE, SND_PCM_STREAM_PLAYBACK, 0);

    if (pcmResult < 0) {
        fmt::print("Can't open PCM device '{}': {}\n", PCM_DEVICE, snd_strerror(pcmResult));

        return 1;
    }

    // ----------------------------
    // Configure hardware settings.

    snd_pcm_hw_params_t *params;

    // Allocate object & get defaults.
    snd_pcm_hw_params_alloca(&params);
    snd_pcm_hw_params_any(pcm_handle, params);

    pcmResult = snd_pcm_hw_params_set_access(pcm_handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);

    if (pcmResult < 0) {
        fmt::print("Failed to set access mode: {}\n", snd_strerror(pcmResult));

        return 1;
    }

    // Format "signed 16 bit Little Endian".
    pcmResult = snd_pcm_hw_params_set_format(pcm_handle, params, SND_PCM_FORMAT_FLOAT_LE);

    if (pcmResult < 0) {
        fmt::print("Failed to set format: {}\n", snd_strerror(pcmResult));

        return 1;
    }

    pcmResult = snd_pcm_hw_params_set_channels(pcm_handle, params, channels);

    if (pcmResult < 0) {
        fmt::print("Failed to set number of channels: {}\n", snd_strerror(pcmResult));

        return 1;
    }

    pcmResult = snd_pcm_hw_params_set_rate_near(pcm_handle, params, &rate, 0);

    if (pcmResult < 0) {
        fmt::print("Failed to set rate: {}\n", snd_strerror(pcmResult));

        return 1;
    }

    pcmResult = snd_pcm_hw_params(pcm_handle, params);

    if (pcmResult < 0) {
        fmt::print("Failed to set hardware params: {}\n", snd_strerror(pcmResult));

        return 1;
    }

    // ---------------------------------
    // Output some hardware information.

    fmt::print("PCM name: '{}'\n", snd_pcm_name(pcm_handle));
    fmt::print("PCM state: {}\n", snd_pcm_state_name(snd_pcm_state(pcm_handle)));

    unsigned int hwChannels;
    snd_pcm_hw_params_get_channels(params, &hwChannels);
    fmt::print("channels: {}\n", hwChannels);

    unsigned int hwRate;
    snd_pcm_hw_params_get_rate(params, &hwRate, 0);
    fmt::print("Hardware rate: {} HZ\n", hwRate);

    // -----------------------------
    // Create buffer for stdin data.

    snd_pcm_uframes_t framesPerPeriod;
    snd_pcm_hw_params_get_period_size(params, &framesPerPeriod, 0);

    fmt::print("Hardware period size: {} frames\n", framesPerPeriod);

    // -----------------------------------
    // Play file data received from stdin.

    unsigned int hwPeriodTime;
    snd_pcm_hw_params_get_period_time(params, &hwPeriodTime, nullptr);

    // Period time is apparently in microseconds.
    fmt::print("Hardware period time: {} us\n", hwPeriodTime);

    float *fileData = inFile.data();

    std::size_t samplesPerPeriod = framesPerPeriod * channels;
    // NOTE: This could potentially truncate audio by very small amount.
    std::size_t numPeriods = inFile.dataLength() / samplesPerPeriod;

    fmt::print("\nPlaying sound data from file...\n");

    for (std::size_t i = 0; i < numPeriods; i++) {
        // NOTE: This knows how many bytes each frame contains.
        // This will buffer frames for playback by the sound card.
        // Evidence suggests we stay well ahead of playback here.
        snd_pcm_sframes_t framesWritten = snd_pcm_writei(pcm_handle, fileData, framesPerPeriod);

        if (framesWritten == -EPIPE) {
            // An underrun has occurred, which happens when "an application
            // does not feed new samples in time to alsa-lib (due CPU usage)".
            fmt::print("An underrun has occurred while writing to device.\n");

            snd_pcm_prepare(pcm_handle);
        } else if (framesWritten < 0) {
            // The docs say this could be -EBADFD or -ESTRPIPE.
            fmt::print("Failed to write to PCM device: {}\n", snd_strerror(static_cast<int>(framesWritten)));
        }

        // Increment data pointer to start of next frame.
        fileData += samplesPerPeriod;
    }

    // -----------------------
    // Clean up and shut down.

    snd_pcm_drain(pcm_handle);
    snd_pcm_close(pcm_handle);

    fmt::print("Playback complete.\n");

    // -----
    // Done.

    return 0;
}
