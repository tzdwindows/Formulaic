#pragma once

#include <Formulaic/core/export.hpp>
#include <Formulaic/parser/expression.hpp>
#include <complex>
#include <span>
#include <vector>

namespace formulaic::math {

enum class WindowType {
    Rectangular,
    Hann,
    Hamming,
    Blackman,
    Bartlett,
    FlatTop
};

class FORMULAIC_API FFT {
public:
    // 1D Radix-2 Cooley-Tukey Fast Fourier Transform
    [[nodiscard]] static std::vector<std::complex<double>> fft(
        std::span<const std::complex<double>> input
    );

    // 1D Inverse Fast Fourier Transform
    [[nodiscard]] static std::vector<std::complex<double>> ifft(
        std::span<const std::complex<double>> input
    );

    // Real-valued input FFT convenience
    [[nodiscard]] static std::vector<std::complex<double>> fft_real(
        std::span<const double> input
    );

    // 2D Fast Fourier Transform for width x height complex matrix
    [[nodiscard]] static std::vector<std::complex<double>> fft2d(
        std::span<const std::complex<double>> grid,
        size_t width,
        size_t height
    );

    // 2D Inverse Fast Fourier Transform
    [[nodiscard]] static std::vector<std::complex<double>> ifft2d(
        std::span<const std::complex<double>> grid,
        size_t width,
        size_t height
    );

    // Apply window function to a real signal in-place
    static void apply_window(
        std::span<double> buffer,
        WindowType window
    ) noexcept;

    // Compute Power Spectral Density (|X[k]|^2 / N)
    [[nodiscard]] static std::vector<double> power_spectrum(
        std::span<const std::complex<double>> fft_data
    );

    // Compute Magnitude Spectrum (|X[k]|)
    [[nodiscard]] static std::vector<double> magnitude_spectrum(
        std::span<const std::complex<double>> fft_data
    );

    // Sample an expression over [t_start, t_end] with N samples and compute magnitude spectrum
    [[nodiscard]] static std::vector<double> sample_and_spectrum(
        const Expression& expr,
        double t_start,
        double t_end,
        size_t sample_count,
        WindowType window = WindowType::Hann
    );
};

} // namespace formulaic::math
