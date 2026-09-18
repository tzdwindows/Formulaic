#include <Formulaic/render/color.hpp>
#include <cmath>

namespace formulaic {

Color sample_colormap(ColormapType type, double t) noexcept {
    t = std::clamp(t, 0.0, 1.0);

    switch (type) {
        case ColormapType::Grayscale: {
            const auto v = static_cast<uint8_t>(t * 255.0);
            return Color(v, v, v, 255);
        }

        case ColormapType::Viridis: {
            // Cubic polynomial approximation for Matplotlib Viridis
            const double r = -0.019 + t * (0.835 + t * (-1.334 + t * 0.963));
            const double g =  0.005 + t * (1.399 + t * (-0.979 + t * 0.551));
            const double b =  0.287 + t * (1.758 + t * (-3.296 + t * 1.579));
            return Color(
                static_cast<uint8_t>(std::clamp(r, 0.0, 1.0) * 255.0),
                static_cast<uint8_t>(std::clamp(g, 0.0, 1.0) * 255.0),
                static_cast<uint8_t>(std::clamp(b, 0.0, 1.0) * 255.0),
                255
            );
        }

        case ColormapType::Plasma: {
            const double r = 0.05 + t * (2.1 + t * (-2.2 + t * 1.05));
            const double g = 0.01 + t * (0.05 + t * (1.5 - t * 0.6));
            const double b = 0.53 + t * (0.8 - t * 1.05);
            return Color(
                static_cast<uint8_t>(std::clamp(r, 0.0, 1.0) * 255.0),
                static_cast<uint8_t>(std::clamp(g, 0.0, 1.0) * 255.0),
                static_cast<uint8_t>(std::clamp(b, 0.0, 1.0) * 255.0),
                255
            );
        }

        case ColormapType::Coolwarm: {
            // Blue -> White -> Red
            if (t < 0.5) {
                const double sub = t * 2.0;
                return Color::lerp(Color(59, 76, 192, 255), Color(221, 221, 221, 255), sub);
            } else {
                const double sub = (t - 0.5) * 2.0;
                return Color::lerp(Color(221, 221, 221, 255), Color(180, 4, 38, 255), sub);
            }
        }

        case ColormapType::Jet: {
            // Classic 4-segment Jet colormap
            double r = std::clamp(1.5 - std::abs(4.0 * t - 3.0), 0.0, 1.0);
            double g = std::clamp(1.5 - std::abs(4.0 * t - 2.0), 0.0, 1.0);
            double b = std::clamp(1.5 - std::abs(4.0 * t - 1.0), 0.0, 1.0);
            return Color(
                static_cast<uint8_t>(r * 255.0),
                static_cast<uint8_t>(g * 255.0),
                static_cast<uint8_t>(b * 255.0),
                255
            );
        }
    }

    return Color::White;
}

} // namespace formulaic
