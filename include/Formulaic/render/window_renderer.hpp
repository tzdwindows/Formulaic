#pragma once

#include <Formulaic/core/export.hpp>
#include <Formulaic/hooks/pipeline_hooks.hpp>
#include <Formulaic/render/framebuffer.hpp>
#include <Formulaic/render/viewport.hpp>
#include <functional>
#include <memory>
#include <string>

namespace formulaic {

class FORMULAIC_API IWindowRenderer {
public:
    virtual ~IWindowRenderer() = default;

    // Attaches to an existing native window handle (e.g. HWND on Windows)
    virtual bool attach(void* native_handle) = 0;

    // Detaches from current window handle
    virtual void detach() = 0;

    // Handles window resize event
    virtual void on_resize(int new_width, int new_height) = 0;

    // Triggers rendering pipeline (executes PreRender, user render callback, PostRender)
    virtual void render(double time_t = 0.0) = 0;

    // Blits the internal double-buffer to the native window surface (e.g. via BitBlt/DIBSection)
    virtual void present() = 0;

    // Configures user drawing logic invoked between PreRender and PostRender hooks
    using RenderCallback = std::function<void(FrameBuffer& fb, const Viewport& vp, double time)>;
    virtual void set_render_callback(RenderCallback callback) = 0;

    // Accessors
    [[nodiscard]] virtual FrameBuffer& framebuffer() noexcept = 0;
    [[nodiscard]] virtual const FrameBuffer& framebuffer() const noexcept = 0;
    [[nodiscard]] virtual Viewport& viewport() noexcept = 0;
    [[nodiscard]] virtual const Viewport& viewport() const noexcept = 0;
    [[nodiscard]] virtual PipelineHooks& hooks() noexcept = 0;
    [[nodiscard]] virtual const PipelineHooks& hooks() const noexcept = 0;

    [[nodiscard]] virtual void* native_handle() const noexcept = 0;
    [[nodiscard]] virtual bool is_attached() const noexcept = 0;
};

// Factory to instantiate platform-specific window renderer
FORMULAIC_API std::unique_ptr<IWindowRenderer> create_window_renderer();

#if defined(_WIN32)
// Windows Win32 helper utilities for creating and running managed test/demo windows
struct Win32WindowDesc {
    std::string title{"Formulaic Math Visualizer"};
    int width{960};
    int height{640};
    bool show{true};
};

struct Win32WindowHandle {
    void* hwnd{nullptr};
    void* hinstance{nullptr};
};

FORMULAIC_API Win32WindowHandle create_win32_window(
    const Win32WindowDesc& desc,
    IWindowRenderer* renderer
);

FORMULAIC_API void destroy_win32_window(Win32WindowHandle handle);
FORMULAIC_API void run_win32_message_loop(IWindowRenderer* renderer, bool run_animation = true);
#endif

} // namespace formulaic
