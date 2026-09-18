#include <Formulaic/render/frame_stream.hpp>
#include <Formulaic/render/image_export.hpp>
#include <iomanip>
#include <sstream>

namespace formulaic {

FrameStream::FrameStream(int width, int height, double start_time, double end_time, double fps)
    : width_(width), height_(height), start_time_(start_time), end_time_(end_time), fps_(fps) {}

void FrameStream::set_time_range(double start_time, double end_time, double fps) noexcept {
    start_time_ = start_time;
    end_time_ = end_time;
    fps_ = std::max(1.0, fps);
}

size_t FrameStream::total_frames() const noexcept {
    if (end_time_ < start_time_ || fps_ <= 0.0) return 0;
    const double dur = end_time_ - start_time_;
    return static_cast<size_t>(std::round(dur * fps_)) + 1;
}

double FrameStream::duration() const noexcept {
    return std::max(0.0, end_time_ - start_time_);
}

void FrameStream::generate(
    const std::function<void(FrameBuffer& fb, double time, size_t frame_idx)>& render_func,
    const FrameCallback& on_frame
) {
    const size_t count = total_frames();
    if (count == 0 || !render_func || !on_frame) return;

    const double dt = (count > 1) ? (end_time_ - start_time_) / (count - 1) : 0.0;
    auto shared_buffer = std::make_shared<FrameBuffer>(width_, height_);

    for (size_t i = 0; i < count; ++i) {
        const double current_t = start_time_ + i * dt;
        render_func(*shared_buffer, current_t, i);

        Frame frame{i, current_t, shared_buffer};
        bool proceed = on_frame(frame);
        if (!proceed) {
            break; // Caller requested stop
        }
    }
}

Result<size_t> FrameStream::dump_to_directory(
    const std::filesystem::path& output_dir,
    const std::string& prefix,
    const std::function<void(FrameBuffer& fb, double time, size_t frame_idx)>& render_func
) {
    if (!std::filesystem::exists(output_dir)) {
        std::filesystem::create_directories(output_dir);
    }

    size_t exported = 0;
    generate(render_func, [&](const Frame& frame) -> bool {
        std::ostringstream ss;
        ss << prefix << "_" << std::setw(5) << std::setfill('0') << frame.index << ".bmp";
        std::filesystem::path target = output_dir / ss.str();
        auto res = ImageExport::save_bmp(*frame.buffer, target);
        if (!res) {
            return false;
        }
        exported++;
        return true;
    });

    return exported;
}

} // namespace formulaic
