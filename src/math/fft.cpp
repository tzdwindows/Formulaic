#include <Formulaic/math/fft.hpp>
#include <cmath>
#include <numbers>

namespace formulaic::math {

namespace {

constexpr double kPi = 3.14159265358979323846;

size_t next_power_of_two(size_t n) noexcept {
    if (n <= 1) return 1;
    size_t p = 1;
    while (p < n) {
        p <<= 1;
    }
    return p;
}

void fft_internal(std::vector<std::complex<double>>& a, bool invert) {
    const size_t n = a.size();
    if (n <= 1) return;

    // Bit reversal permutation
    for (size_t i = 1, j = 0; i < n; ++i) {
        size_t bit = n >> 1;
        for (; j & bit; bit >>= 1) {
            j ^= bit;
        }
        j ^= bit;
        if (i < j) {
            std::swap(a[i], a[j]);
        }
    }

    // Cooley-Tukey butterflies
    for (size_t len = 2; len <= n; len <<= 1) {
        const double angle = (invert ? 2.0 : -2.0) * kPi / static_cast<double>(len);
        const std::complex<double> wlen(std::cos(angle), std::sin(angle));

        for (size_t i = 0; i < n; i += len) {
            std::complex<double> w(1.0, 0.0);
            for (size_t j = 0; j < len / 2; ++j) {
                const std::complex<double> u = a[i + j];
                const std::complex<double> v = a[i + j + len / 2] * w;
                a[i + j] = u + v;
                a[i + j + len / 2] = u - v;
                w *= wlen;
            }
        }
    }

    if (invert) {
        const double inv_n = 1.0 / static_cast<double>(n);
        for (auto& x : a) {
            x *= inv_n;
        }
    }
}

} // anonymous namespace

std::vector<std::complex<double>> FFT::fft(std::span<const std::complex<double>> input) {
    if (input.empty()) return {};

    const size_t n = next_power_of_two(input.size());
    std::vector<std::complex<double>> data(n, {0.0, 0.0});
    for (size_t i = 0; i < input.size(); ++i) {
        data[i] = input[i];
    }

    fft_internal(data, false);
    return data;
}

std::vector<std::complex<double>> FFT::ifft(std::span<const std::complex<double>> input) {
    if (input.empty()) return {};

    const size_t n = next_power_of_two(input.size());
    std::vector<std::complex<double>> data(n, {0.0, 0.0});
    for (size_t i = 0; i < input.size(); ++i) {
        data[i] = input[i];
    }

    fft_internal(data, true);
    return data;
}

std::vector<std::complex<double>> FFT::fft_real(std::span<const double> input) {
    if (input.empty()) return {};

    const size_t n = next_power_of_two(input.size());
    std::vector<std::complex<double>> data(n, {0.0, 0.0});
    for (size_t i = 0; i < input.size(); ++i) {
        data[i] = std::complex<double>(input[i], 0.0);
    }

    fft_internal(data, false);
    return data;
}

std::vector<std::complex<double>> FFT::fft2d(
    std::span<const std::complex<double>> grid,
    size_t width,
    size_t height
) {
    if (width == 0 || height == 0 || grid.size() < width * height) return {};

    const size_t pow2_w = next_power_of_two(width);
    const size_t pow2_h = next_power_of_two(height);

    std::vector<std::complex<double>> data(pow2_w * pow2_h, {0.0, 0.0});
    for (size_t y = 0; y < height; ++y) {
        for (size_t x = 0; x < width; ++x) {
            data[y * pow2_w + x] = grid[y * width + x];
        }
    }

    // FFT on each row
    std::vector<std::complex<double>> row_buf(pow2_w);
    for (size_t y = 0; y < pow2_h; ++y) {
        for (size_t x = 0; x < pow2_w; ++x) {
            row_buf[x] = data[y * pow2_w + x];
        }
        fft_internal(row_buf, false);
        for (size_t x = 0; x < pow2_w; ++x) {
            data[y * pow2_w + x] = row_buf[x];
        }
    }

    // FFT on each column
    std::vector<std::complex<double>> col_buf(pow2_h);
    for (size_t x = 0; x < pow2_w; ++x) {
        for (size_t y = 0; y < pow2_h; ++y) {
            col_buf[y] = data[y * pow2_w + x];
        }
        fft_internal(col_buf, false);
        for (size_t y = 0; y < pow2_h; ++y) {
            data[y * pow2_w + x] = col_buf[y];
        }
    }

    return data;
}

std::vector<std::complex<double>> FFT::ifft2d(
    std::span<const std::complex<double>> grid,
    size_t width,
    size_t height
) {
    if (width == 0 || height == 0 || grid.size() < width * height) return {};

    const size_t pow2_w = next_power_of_two(width);
    const size_t pow2_h = next_power_of_two(height);

    std::vector<std::complex<double>> data(pow2_w * pow2_h, {0.0, 0.0});
    for (size_t y = 0; y < height; ++y) {
        for (size_t x = 0; x < width; ++x) {
            data[y * pow2_w + x] = grid[y * width + x];
        }
    }

    // IFFT on each row
    std::vector<std::complex<double>> row_buf(pow2_w);
    for (size_t y = 0; y < pow2_h; ++y) {
        for (size_t x = 0; x < pow2_w; ++x) {
            row_buf[x] = data[y * pow2_w + x];
        }
        fft_internal(row_buf, true);
        for (size_t x = 0; x < pow2_w; ++x) {
            data[y * pow2_w + x] = row_buf[x];
        }
    }

    // IFFT on each column
    std::vector<std::complex<double>> col_buf(pow2_h);
    for (size_t x = 0; x < pow2_w; ++x) {
        for (size_t y = 0; y < pow2_h; ++y) {
            col_buf[y] = data[y * pow2_w + x];
        }
        fft_internal(col_buf, true);
        for (size_t y = 0; y < pow2_h; ++y) {
            data[y * pow2_w + x] = col_buf[y];
        }
    }

    return data;
}

void FFT::apply_window(std::span<double> buffer, WindowType window) noexcept {
    const size_t n = buffer.size();
    if (n <= 1 || window == WindowType::Rectangular) return;

    const double denom = static_cast<double>(n - 1);
    for (size_t i = 0; i < n; ++i) {
        const double frac = static_cast<double>(i) / denom;
        double w = 1.0;

        switch (window) {
            case WindowType::Hann:
                w = 0.5 * (1.0 - std::cos(2.0 * kPi * frac));
                break;
            case WindowType::Hamming:
                w = 0.54 - 0.46 * std::cos(2.0 * kPi * frac);
                break;
            case WindowType::Blackman:
                w = 0.42 - 0.5 * std::cos(2.0 * kPi * frac) + 0.08 * std::cos(4.0 * kPi * frac);
                break;
            case WindowType::Bartlett:
                w = 1.0 - std::abs(2.0 * frac - 1.0);
                break;
            case WindowType::FlatTop:
                w = 0.21557895 - 0.41663158 * std::cos(2.0 * kPi * frac)
                               + 0.277263158 * std::cos(4.0 * kPi * frac)
                               - 0.083578947 * std::cos(6.0 * kPi * frac)
                               + 0.006947368 * std::cos(8.0 * kPi * frac);
                break;
            case WindowType::Rectangular:
                w = 1.0;
                break;
        }

        buffer[i] *= w;
    }
}

std::vector<double> FFT::power_spectrum(std::span<const std::complex<double>> fft_data) {
    if (fft_data.empty()) return {};

    const size_t n = fft_data.size();
    const double inv_n = 1.0 / static_cast<double>(n);
    std::vector<double> ps(n / 2 + 1);

    for (size_t i = 0; i < ps.size(); ++i) {
        const double re = fft_data[i].real();
        const double im = fft_data[i].imag();
        ps[i] = (re * re + im * im) * inv_n;
    }

    return ps;
}

std::vector<double> FFT::magnitude_spectrum(std::span<const std::complex<double>> fft_data) {
    if (fft_data.empty()) return {};

    const size_t half = fft_data.size() / 2 + 1;
    std::vector<double> mag(half);

    for (size_t i = 0; i < half; ++i) {
        mag[i] = std::abs(fft_data[i]);
    }

    return mag;
}

std::vector<double> FFT::sample_and_spectrum(
    const Expression& expr,
    double t_start,
    double t_end,
    size_t sample_count,
    WindowType window
) {
    if (sample_count < 2) return {};

    std::vector<double> samples(sample_count);
    const double dt = (t_end - t_start) / static_cast<double>(sample_count);

    for (size_t i = 0; i < sample_count; ++i) {
        const double t = t_start + static_cast<double>(i) * dt;
        samples[i] = expr.eval(t);
    }

    apply_window(samples, window);

    auto complex_fft = fft_real(samples);
    return magnitude_spectrum(complex_fft);
}

} // namespace formulaic::math
