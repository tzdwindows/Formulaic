#pragma once

#include <Formulaic/core/error.hpp>
#include <Formulaic/core/export.hpp>
#include <Formulaic/render/framebuffer.hpp>
#include <filesystem>
#include <functional>
#include <memory>
#include <string>

namespace formulaic {

struct FORMULAIC_API Frame {
    size_t index{0};
    double timestamp{0.0};
    std::shared_ptr<FrameBuffer> buffer;
};

class FORMULAIC_API FrameStream {
public:
    using FrameCallback = std::function<bool(const Frame& frame)>;

    FrameStream(int width, int height, double start_time = 0.0, double end_time = 1.0, double fps = 60.0);

    void set_time_range(double start_time, double end_time, double fps) noexcept;
    [[nodiscard]] size_t total_frames() const noexcept;
    [[nodiscard]] double duration() const noexcept;
    [[nodiscard]] double fps() const noexcept { return fps_; }
    [[nodiscard]] double delta_time() const noexcept { return 1.0 / fps_; }

    // Generates frames iteratively, invoking a callback for each frame (supports early termination)
    void generate(const std::function<void(FrameBuffer& fb, double time, size_t frame_idx)>& render_func,
                   const FrameCallback& on_frame);

    // Convenience method to dump frame stream to a directory as sequentially named images
    Result<size_t> dump_to_directory(
        const std::filesystem::path& output_dir,
        const std::string& prefix,
        const std::function<void(FrameBuffer& fb, double time, size_t frame_idx)>& render_func
    );

private:
    int width_{800};
    int height_{600};
    double start_time_{0.0};
    double end_time_{1.0};
    double fps_{60.0};
};

} // namespace formulaic
