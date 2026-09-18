#include <Formulaic/math/calculus.hpp>
#include <Formulaic/math/fft.hpp>
#include <Formulaic/parser/expression.hpp>
#include <Formulaic/render/color.hpp>
#include <Formulaic/render/framebuffer.hpp>
#include <Formulaic/render/image_export.hpp>
#include <Formulaic/render/raster_engine.hpp>
#include <Formulaic/render/viewport.hpp>

#include <cmath>
#include <complex>
#include <filesystem>
#include <iostream>
#include <numbers>
#include <string>
#include <vector>

#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            std::cerr << "Assertion failed: [" << #cond << "] " << (msg) \
                      << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
            return 1; \
        } \
    } while (0)

#define TEST_ASSERT_NEAR(actual, expected, tol, msg) \
    do { \
        double a = (actual); \
        double e = (expected); \
        if (std::abs(a - e) > (tol)) { \
            std::cerr << "Assertion failed: " << (msg) << " | Expected " \
                      << e << ", got " << a << " (diff=" << std::abs(a - e) << ")" \
                      << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
            return 1; \
        } \
    } while (0)

static bool render_custom_formula_to_image(
    const std::string& formula_text,
    const std::filesystem::path& out_path,
    int width = 800,
    int height = 600,
    double x_min = -5.0,
    double x_max = 5.0,
    double y_min = -5.0,
    double y_max = 5.0
) {
    auto expr_res = formulaic::Expression::parse(formula_text);
    if (!expr_res) {
        std::cerr << "Parse error: " << expr_res.error().format() << std::endl;
        return false;
    }

    const auto& expr = expr_res.value();
    formulaic::FrameBuffer fb(width, height, formulaic::Color::BackgroundDark);
    formulaic::Viewport vp(width, height, formulaic::Rect2D(x_min, x_max, y_min, y_max));
    formulaic::RasterEngine engine;

    // Draw coordinate grid
    formulaic::GridStyle grid_style;
    grid_style.show_grid = true;
    grid_style.show_axes = true;
    grid_style.show_labels = true;
    engine.render_grid(fb, vp, grid_style);

    // Determine whether to plot as explicit 1D curve or 2D scalar field/heatmap
    // If expression references 'y', render as 2D scalar field heatmap; otherwise as 1D curve
    bool has_y = false;
    for (const auto& var : expr.variables()) {
        if (var == "y") {
            has_y = true;
            break;
        }
    }

    if (has_y) {
        engine.plot_scalar_field(fb, vp, expr, formulaic::ColormapType::Viridis, -2.0, 2.0);
    } else {
        engine.plot_explicit(fb, vp, expr, formulaic::Color::NeonBlue, 2.5);
    }

    // Watermark title
    std::string title = "Formulaic Custom Input: " + formula_text.substr(0, 45);
    if (formula_text.length() > 45) title += "...";
    fb.draw_text(16, 16, title, formulaic::Color::White);

    auto save_res = formulaic::ImageExport::save_bmp(fb, out_path);
    if (!save_res || !save_res.value()) {
        std::cerr << "Failed to export BMP to " << out_path.string() << std::endl;
        return false;
    }

    std::cout << "Successfully rendered and exported to: " << out_path.string() << "\n";
    return true;
}

int main(int argc, char* argv[]) {
    const std::filesystem::path output_dir = "test_output";
    std::filesystem::create_directories(output_dir);

    // If custom input arguments or interactive mode requested
    if (argc > 1) {
        std::string arg1 = argv[1];

        if (arg1 == "--help" || arg1 == "-h") {
            std::cout << "Formulaic Custom Input Image Renderer Test\n";
            std::cout << "Usage:\n";
            std::cout << "  test_custom_render                   Run automated test suite (CTest compatible)\n";
            std::cout << "  test_custom_render --interactive     Enter custom formula interactively\n";
            std::cout << "  test_custom_render \"<formula>\"         Render custom formula expression\n";
            std::cout << "  test_custom_render --expr \"<formula>\"  Render custom formula expression\n";
            return 0;
        }

        if (arg1 == "--interactive" || arg1 == "-i") {
            std::cout << "========================================================\n";
            std::cout << "   Formulaic Interactive Expression & Script Renderer   \n";
            std::cout << "========================================================\n";
            std::cout << "Enter your formula or script below.\n";
            std::cout << "You can declare variables with 'let a = ...;' or 'var b = ...;'\n";
            std::cout << "Functions supported: sin, cos, hypot, diff_step, hann, square_wave, chirp, etc.\n";
            std::cout << "Example: let r = hypot(x, y); sin(5 * r) * exp(-0.5 * r)\n";
            std::cout << "--------------------------------------------------------\n";
            std::cout << "Formula: ";

            std::string input_line;
            if (std::getline(std::cin, input_line) && !input_line.empty()) {
                const auto out_file = output_dir / "custom_interactive_render.bmp";
                if (render_custom_formula_to_image(input_line, out_file)) {
                    std::cout << "Done! Open " << out_file.string() << " to view your rendered image.\n";
                    return 0;
                }
                return 1;
            }
            std::cout << "No formula entered. Exiting.\n";
            return 0;
        }

        std::string custom_expr = arg1;
        if (arg1 == "--expr" && argc > 2) {
            custom_expr = argv[2];
        }

        const auto out_file = output_dir / "custom_cli_render.bmp";
        std::cout << "Rendering custom input formula: \"" << custom_expr << "\"\n";
        if (render_custom_formula_to_image(custom_expr, out_file)) {
            return 0;
        }
        return 1;
    }

    // =========================================================================
    // Automated Test Suite (runs when no arguments provided)
    // =========================================================================
    std::cout << "=========================================================\n";
    std::cout << " Running Custom Input & Calculus / FFT Render Tests      \n";
    std::cout << "=========================================================\n";

    // -------------------------------------------------------------------------
    // Test 1: Custom User Script with Variable Declarations (let/var)
    // -------------------------------------------------------------------------
    {
        std::cout << "[Test 1] Custom Script with Variables (let & var)... ";
        const std::string script =
            "let r = hypot(x, y);\n"
            "let theta = atan2(y, x);\n"
            "let envelope = exp(-0.35 * r);\n"
            "sin(6.0 * theta) * envelope;";

        const auto out_file = output_dir / "custom_render_variables.bmp";
        bool ok = render_custom_formula_to_image(script, out_file, 640, 480, -4.0, 4.0, -4.0, 4.0);
        TEST_ASSERT(ok, "Render custom script with let variables");
        TEST_ASSERT(std::filesystem::exists(out_file), "BMP output file exists");
        TEST_ASSERT(std::filesystem::file_size(out_file) > 1000, "BMP output file is non-empty");
        std::cout << "PASSED\n";
    }

    // -------------------------------------------------------------------------
    // Test 2: Custom Numerical Calculus Expression (diff_step, curvature)
    // -------------------------------------------------------------------------
    {
        std::cout << "[Test 2] Custom Calculus Expression & Numerical Verification... ";
        // Numerical derivative of sin(x)*cos(y) with respect to x:
        // diff_step(sin(x+h)*cos(y), sin(x-h)*cos(y), h)
        const std::string calc_script =
            "let h = 0.005;\n"
            "let fp = sin(x + h) * cos(y);\n"
            "let fm = sin(x - h) * cos(y);\n"
            "diff_step(fp, fm, h);";

        const auto out_file = output_dir / "custom_render_calculus.bmp";
        bool ok = render_custom_formula_to_image(calc_script, out_file, 640, 480, -3.14159, 3.14159, -3.14159, 3.14159);
        TEST_ASSERT(ok, "Render calculus script");
        TEST_ASSERT(std::filesystem::exists(out_file), "Calculus BMP output exists");

        // Verify that numerical diff_step accurately approximates analytical cos(x)*cos(y)
        auto expr = formulaic::Expression::parse(calc_script);
        TEST_ASSERT(expr.has_value(), "Parse calculus script");
        const double x_val = 0.75;
        const double y_val = 0.50;
        const double approx_deriv = expr->eval(x_val, y_val);
        const double exact_deriv = std::cos(x_val) * std::cos(y_val);
        TEST_ASSERT_NEAR(approx_deriv, exact_deriv, 1e-4, "Numerical derivative matches analytical derivative");

        std::cout << "PASSED\n";
    }

    // -------------------------------------------------------------------------
    // Test 3: FFT Windowing & Waveform Rendering (hann, square_wave, chirp)
    // -------------------------------------------------------------------------
    {
        std::cout << "[Test 3] Spectral Windows & Waveforms (hann & triangle_wave)... ";
        const std::string wave_script =
            "let norm = clamp((x + 5.0) / 10.0, 0.0, 1.0);\n"
            "let win = hann(norm);\n"
            "let wave = triangle_wave(norm * 4.0);\n"
            "win * wave;";

        const auto out_file = output_dir / "custom_render_fft_wave.bmp";
        bool ok = render_custom_formula_to_image(wave_script, out_file, 800, 400, -6.0, 6.0, -1.5, 1.5);
        TEST_ASSERT(ok, "Render FFT windowed waveform");
        TEST_ASSERT(std::filesystem::exists(out_file), "Waveform BMP output exists");

        std::cout << "PASSED\n";
    }

    // -------------------------------------------------------------------------
    // Test 4: C++ Calculus & FFT Engine Direct API Verification
    // -------------------------------------------------------------------------
    {
        std::cout << "[Test 4] C++ Calculus & FFT Engine Analytical Verification... ";

        // 1. Numerical Differentiation 5-point stencil: d/dx [sin(x)] at x = pi/3 is cos(pi/3) = 0.5
        auto sin_expr = formulaic::Expression::parse("sin(x)");
        TEST_ASSERT(sin_expr.has_value(), "Parse sin(x)");
        const double pi_over_3 = 3.14159265358979323846 / 3.0;
        const double d_sin = sin_expr->differentiate(pi_over_3, 1e-5);
        TEST_ASSERT_NEAR(d_sin, 0.5, 1e-7, "5-point stencil derivative of sin(pi/3) is 0.5");

        // 2. Numerical Integration Simpson's Rule: int_0^2 3*x^2 dx = [x^3]_0^2 = 8.0
        auto poly_expr = formulaic::Expression::parse("3 * x^2");
        TEST_ASSERT(poly_expr.has_value(), "Parse 3*x^2");
        const double integral_val = poly_expr->integrate(0.0, 2.0, 1000);
        TEST_ASSERT_NEAR(integral_val, 8.0, 1e-6, "Simpson's rule integral of 3x^2 on [0, 2] is 8.0");

        // 3. 1D FFT & IFFT round-trip reconstruction
        std::vector<std::complex<double>> signal = {
            {1.0, 0.0}, {2.0, 0.0}, {-1.0, 0.0}, {3.0, 0.0},
            {0.5, 0.0}, {-2.0, 0.0}, {1.5, 0.0}, {-0.5, 0.0}
        };
        auto freq_data = formulaic::math::FFT::fft(signal);
        auto reconstructed = formulaic::math::FFT::ifft(freq_data);
        for (size_t i = 0; i < signal.size(); ++i) {
            TEST_ASSERT_NEAR(reconstructed[i].real(), signal[i].real(), 1e-12, "IFFT real reconstruction");
            TEST_ASSERT_NEAR(reconstructed[i].imag(), signal[i].imag(), 1e-12, "IFFT imag reconstruction");
        }

        // 4. 2D FFT & 2D IFFT round-trip reconstruction
        std::vector<std::complex<double>> grid2d = {
            {1.0, 0.0}, {2.0, 0.0},
            {3.0, 0.0}, {4.0, 0.0}
        };
        auto freq2d = formulaic::math::FFT::fft2d(grid2d, 2, 2);
        auto recon2d = formulaic::math::FFT::ifft2d(freq2d, 2, 2);
        for (size_t i = 0; i < 4; ++i) {
            TEST_ASSERT_NEAR(recon2d[i].real(), grid2d[i].real(), 1e-12, "2D IFFT reconstruction");
        }

        // 5. Frequency peak detection with sample_and_spectrum:
        // A pure tone at 8 Hz in a 1-second sample should have peak at index 8
        auto tone_expr = formulaic::Expression::parse("sin(2.0 * pi * 8.0 * x)");
        TEST_ASSERT(tone_expr.has_value(), "Parse 8Hz tone");
        auto spectrum = tone_expr->compute_spectrum(0.0, 1.0, 128);
        TEST_ASSERT(!spectrum.empty(), "Spectrum computed");

        // Find argmax
        size_t max_bin = 0;
        double max_val = 0.0;
        for (size_t i = 1; i < spectrum.size(); ++i) { // skip DC
            if (spectrum[i] > max_val) {
                max_val = spectrum[i];
                max_bin = i;
            }
        }
        TEST_ASSERT(max_bin == 8, "Peak detected at 8 Hz bin");

        std::cout << "PASSED\n";
    }

    std::cout << "\n>>> All Custom Input & Calculus / FFT Render Tests PASSED successfully! <<<\n";
    return 0;
}
