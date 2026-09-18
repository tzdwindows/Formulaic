#pragma once

#include <Formulaic/core/export.hpp>
#include <Formulaic/core/types.hpp>
#include <Formulaic/render/color.hpp>
#include <functional>

namespace formulaic {

class FrameBuffer;
class Viewport;

// Hook executed before the main graph rasterization
using PreRenderHook = std::function<void(FrameBuffer& fb, const Viewport& vp, double time)>;

// Hook executed after graph rasterization (for HUD, overlays, crosshairs)
using PostRenderHook = std::function<void(FrameBuffer& fb, const Viewport& vp, double time)>;

// Hook to customize coordinate transformation from world space to screen space
// (e.g. logarithmic, polar, fish-eye, or hyperbolic projections)
using CoordinateTransformHook = std::function<Point2D(const Point2D& world_pt, const Viewport& vp)>;

// Hook to customize pixel color calculation (e.g. dynamic shading, procedural lighting, heightmaps)
using PixelShaderHook = std::function<Color(
    double world_x,
    double world_y,
    double evaluated_val,
    const Color& base_color,
    double time
)>;

} // namespace formulaic
