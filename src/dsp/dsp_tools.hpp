#ifndef DSP_TOOLS_H_
#define DSP_TOOLS_H_

#include <array>
#include <cmath>
#include <cstddef>
#include <numbers>

namespace dsp {

template <size_t WINDOW_SIZE>
static std::array<double, WINDOW_SIZE> makeHannWindow() {
    std::array<double, WINDOW_SIZE> window{};
    double PI = std::numbers::pi_v<double>;
    for (size_t i = 0; i < WINDOW_SIZE; i++) {
        double sinTerm = std::sin(PI * i / static_cast<double>(WINDOW_SIZE));
        window[i] = sinTerm * sinTerm;
    }
    return window;
}

} // namespace dsp

#endif // DSP_TOOLS_H_
