#pragma once

#include <Formulaic/core/export.hpp>
#include <Formulaic/hooks/hooks.hpp>
#include <vector>

namespace formulaic {

class FORMULAIC_API PipelineHooks {
public:
    PipelineHooks() = default;

    void add_pre_render_hook(PreRenderHook hook) {
        pre_render_hooks_.push_back(std::move(hook));
    }

    void add_post_render_hook(PostRenderHook hook) {
        post_render_hooks_.push_back(std::move(hook));
    }

    void set_coordinate_transform_hook(CoordinateTransformHook hook) {
        coord_transform_hook_ = std::move(hook);
    }

    void set_pixel_shader_hook(PixelShaderHook hook) {
        pixel_shader_hook_ = std::move(hook);
    }

    void clear_all() noexcept {
        pre_render_hooks_.clear();
        post_render_hooks_.clear();
        coord_transform_hook_ = nullptr;
        pixel_shader_hook_ = nullptr;
    }

    // Execution helpers
    void execute_pre_render(FrameBuffer& fb, const Viewport& vp, double time) const {
        for (const auto& hook : pre_render_hooks_) {
            if (hook) hook(fb, vp, time);
        }
    }

    void execute_post_render(FrameBuffer& fb, const Viewport& vp, double time) const {
        for (const auto& hook : post_render_hooks_) {
            if (hook) hook(fb, vp, time);
        }
    }

    [[nodiscard]] bool has_coord_transform_hook() const noexcept {
        return static_cast<bool>(coord_transform_hook_);
    }

    [[nodiscard]] Point2D transform_coordinate(const Point2D& world_pt, const Viewport& vp) const {
        if (coord_transform_hook_) {
            return coord_transform_hook_(world_pt, vp);
        }
        return world_pt;
    }

    [[nodiscard]] bool has_pixel_shader_hook() const noexcept {
        return static_cast<bool>(pixel_shader_hook_);
    }

    [[nodiscard]] Color shade_pixel(double wx, double wy, double val, const Color& base_color, double time) const {
        if (pixel_shader_hook_) {
            return pixel_shader_hook_(wx, wy, val, base_color, time);
        }
        return base_color;
    }

    [[nodiscard]] size_t pre_hook_count() const noexcept { return pre_render_hooks_.size(); }
    [[nodiscard]] size_t post_hook_count() const noexcept { return post_render_hooks_.size(); }

private:
    std::vector<PreRenderHook> pre_render_hooks_;
    std::vector<PostRenderHook> post_render_hooks_;
    CoordinateTransformHook coord_transform_hook_{nullptr};
    PixelShaderHook pixel_shader_hook_{nullptr};
};

} // namespace formulaic
