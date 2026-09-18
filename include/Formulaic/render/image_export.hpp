#pragma once

#include <Formulaic/core/error.hpp>
#include <Formulaic/core/export.hpp>
#include <Formulaic/render/framebuffer.hpp>
#include <filesystem>
#include <vector>

namespace formulaic {

class FORMULAIC_API ImageExport {
public:
    // Exports framebuffer as uncompressed 24-bit or 32-bit Windows Bitmap (.bmp)
    [[nodiscard]] static Result<bool> save_bmp(
        const FrameBuffer& buffer,
        const std::filesystem::path& file_path,
        bool include_alpha = true
    );

    // Encodes framebuffer to in-memory BMP byte buffer
    [[nodiscard]] static std::vector<uint8_t> encode_bmp(
        const FrameBuffer& buffer,
        bool include_alpha = true
    );

    // Exports raw RGBA bytes (width * height * 4 bytes)
    [[nodiscard]] static Result<bool> save_raw_rgba(
        const FrameBuffer& buffer,
        const std::filesystem::path& file_path
    );

    // Exports Netpbm PPM (P6 binary format)
    [[nodiscard]] static Result<bool> save_ppm(
        const FrameBuffer& buffer,
        const std::filesystem::path& file_path
    );
};

} // namespace formulaic
