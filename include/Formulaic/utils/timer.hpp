#pragma once

#include <chrono>

namespace formulaic::utils {

class Timer {
public:
    Timer() noexcept { reset(); }

    void reset() noexcept {
        start_time_ = std::chrono::high_resolution_clock::now();
    }

    [[nodiscard]] double elapsed_seconds() const noexcept {
        auto now = std::chrono::high_resolution_clock::now();
        return std::chrono::duration<double>(now - start_time_).count();
    }

    [[nodiscard]] double elapsed_milliseconds() const noexcept {
        auto now = std::chrono::high_resolution_clock::now();
        return std::chrono::duration<double, std::milli>(now - start_time_).count();
    }

    [[nodiscard]] double elapsed_microseconds() const noexcept {
        auto now = std::chrono::high_resolution_clock::now();
        return std::chrono::duration<double, std::micro>(now - start_time_).count();
    }

private:
    std::chrono::high_resolution_clock::time_point start_time_;
};

} // namespace formulaic::utils
