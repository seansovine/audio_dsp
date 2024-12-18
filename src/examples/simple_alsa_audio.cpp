/// A simple program to play a WAV file using ALSA API.
///
/// This is mostly based on the example of Alessandro Ghedini:
///   https://gist.github.com/ghedo/963382/815c98d1ba0eda1b486eb9d80d9a91a81d995283
///
/// We've also drawn from Alex Via's blog post:
///   https://alexvia.com/post/003_alsa_playback/
/// and the ALSA docs:
///   https://www.alsa-project.org/alsa-doc/alsa-lib/pcm.html
///
/// NOTE: We've hardcoded { sample rate, # channels, # play time }
///       for now, since we know the values to use for our test file.
///
/// We run it with: <BIN_DIR>/SimpleAlsaPlayer < media/Low\ E.wav
///

#include <functional>
#include <alsa/asoundlib.h>

#include <fmt/core.h>

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
    std::function<void()> func;
};

// -------------
// Main program.

int main() {
    // For now hardcode audio params.
    unsigned int rate = 44100;
    unsigned int channels = 1;

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
    pcmResult = snd_pcm_hw_params_set_format(pcm_handle, params, SND_PCM_FORMAT_S16_LE);

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

    // Allocate buffer for reading from stdin and writing to device.
    //
    // NOTE: A frame contains a sample for all channels.
    // We multiply by 2 since the buffer contains
    // bytes, but our sound format is 16-bit.

    std::size_t bytesPerFrame = channels * 2;
    std::size_t bufferSize = framesPerPeriod * bytesPerFrame;
    char *buffer = static_cast<char *>(malloc(bufferSize));

    // Defer freeing buffer to scope exit.
    auto deferFree = Defer{
        [ &buffer ]() {
            fmt::print("Freeing buffer.\n");
            free(buffer);
        }
    };

    // -----------------------------------
    // Play file data received from stdin.

    // Number of seconds to play.
    double seconds = 10.0;

    unsigned int hwPeriodTime;
    snd_pcm_hw_params_get_period_time(params, &hwPeriodTime, nullptr);

    // Period time is apparently in microseconds.
    fmt::print("Hardware period time: {} us\n", hwPeriodTime);

    ssize_t bytesRead;
    snd_pcm_sframes_t framesWritten;

    fmt::print("Playing sound data from stdin...\n");

    std::size_t numPeriods = static_cast<std::size_t>(seconds * 1'000'000) / hwPeriodTime;
    for (std::size_t i = 0; i < numPeriods; i++) {
        bytesRead = read(0, buffer, bufferSize);

        if (bytesRead == 0) {
            fmt::print("No more bytes to read.\n");

            break;
        }

        // NOTE: This knows how many bytes each frame contains.
        // The blocking time of this function is what keeps our
        // writes in sync with the actual sample rate for
        // (approximate?) real-time playback.
        framesWritten = snd_pcm_writei(pcm_handle, buffer, framesPerPeriod);

        if (framesWritten == -EPIPE) {
            // An underrun has occurred, which happens when "an application
            // does not feed new samples in time to alsa-lib (due CPU usage)".
            fmt::print("An underrun has occurred while writing to device.\n");

            snd_pcm_prepare(pcm_handle);
        } else if (framesWritten < 0) {
            // Docs say this could be -EBADFD or -ESTRPIPE.
            fmt::print("Failed to write to PCM device: {}\n", snd_strerror(static_cast<int>(framesWritten)));
        }
    }

    snd_pcm_drain(pcm_handle);
    snd_pcm_close(pcm_handle);

    // -----
    // Done.

    return 0;
}
