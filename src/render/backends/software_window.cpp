#include <Formulaic/render/window_renderer.hpp>

namespace formulaic {

class SoftwareWindowRenderer final : public IWindowRenderer {
public:
    SoftwareWindowRenderer() : framebuffer_(800, 600), viewport_(800, 600) {}

    bool attach(void* native_handle) override {
        native_handle_ = native_handle;
        return true;
    }

    void detach() override {
        native_handle_ = nullptr;
    }

    void on_resize(int new_width, int new_height) override {
        if (new_width <= 0 || new_height <= 0) return;
        framebuffer_.resize(new_width, new_height);
        viewport_.set_size(new_width, new_height);
    }

    void render(double time_t) override {
        hooks_.execute_pre_render(framebuffer_, viewport_, time_t);
        if (render_callback_) {
            render_callback_(framebuffer_, viewport_, time_t);
        }
        hooks_.execute_post_render(framebuffer_, viewport_, time_t);
    }

    void present() override {
        // In software mode, the memory framebuffer is updated in-place
    }

    void set_render_callback(RenderCallback callback) override {
        render_callback_ = std::move(callback);
    }

    [[nodiscard]] FrameBuffer& framebuffer() noexcept override { return framebuffer_; }
    [[nodiscard]] const FrameBuffer& framebuffer() const noexcept override { return framebuffer_; }
    [[nodiscard]] Viewport& viewport() noexcept override { return viewport_; }
    [[nodiscard]] const Viewport& viewport() const noexcept override { return viewport_; }
    [[nodiscard]] PipelineHooks& hooks() noexcept override { return hooks_; }
    [[nodiscard]] const PipelineHooks& hooks() const noexcept override { return hooks_; }

    [[nodiscard]] void* native_handle() const noexcept override { return native_handle_; }
    [[nodiscard]] bool is_attached() const noexcept override { return native_handle_ != nullptr; }

private:
    void* native_handle_{nullptr};
    FrameBuffer framebuffer_;
    Viewport viewport_;
    PipelineHooks hooks_;
    RenderCallback render_callback_;
};

#if !defined(_WIN32)
std::unique_ptr<IWindowRenderer> create_window_renderer() {
    return std::make_unique<SoftwareWindowRenderer>();
}
#endif

} // namespace formulaic
