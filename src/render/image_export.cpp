#include <Formulaic/render/image_export.hpp>
#include <fstream>

namespace formulaic {

#pragma pack(push, 1)
struct BMPFileHeader {
    uint16_t file_type{0x4D42}; // 'BM'
    uint32_t file_size{0};
    uint16_t reserved1{0};
    uint16_t reserved2{0};
    uint32_t offset_data{54};
};

struct BMPInfoHeader {
    uint32_t size{40};
    int32_t  width{0};
    int32_t  height{0};
    uint16_t planes{1};
    uint16_t bit_count{32};
    uint32_t compression{0}; // BI_RGB
    uint32_t size_image{0};
    int32_t  x_pixels_per_meter{2835};
    int32_t  y_pixels_per_meter{2835};
    uint32_t colors_used{0};
    uint32_t colors_important{0};
};
#pragma pack(pop)

std::vector<uint8_t> ImageExport::encode_bmp(const FrameBuffer& buffer, bool include_alpha) {
    const int w = buffer.width();
    const int h = buffer.height();
    const uint16_t bpp = include_alpha ? 32 : 24;
    const uint32_t row_stride = (w * (bpp / 8) + 3) & ~3;
    const uint32_t image_size = row_stride * h;
    const uint32_t total_size = sizeof(BMPFileHeader) + sizeof(BMPInfoHeader) + image_size;

    std::vector<uint8_t> out(total_size);

    BMPFileHeader file_header;
    file_header.file_size = total_size;
    file_header.offset_data = sizeof(BMPFileHeader) + sizeof(BMPInfoHeader);

    BMPInfoHeader info_header;
    info_header.width = w;
    info_header.height = h; // positive means bottom-up format
    info_header.bit_count = bpp;
    info_header.size_image = image_size;

    std::memcpy(out.data(), &file_header, sizeof(BMPFileHeader));
    std::memcpy(out.data() + sizeof(BMPFileHeader), &info_header, sizeof(BMPInfoHeader));

    uint8_t* pixel_dest = out.data() + file_header.offset_data;

    // Bottom-up BMP writing
    for (int y = h - 1; y >= 0; --y) {
        uint8_t* row_ptr = pixel_dest + (h - 1 - y) * row_stride;
        for (int x = 0; x < w; ++x) {
            Color c = buffer.get_pixel(x, y);
            if (include_alpha) {
                // BGRA
                row_ptr[x * 4 + 0] = c.b;
                row_ptr[x * 4 + 1] = c.g;
                row_ptr[x * 4 + 2] = c.r;
                row_ptr[x * 4 + 3] = c.a;
            } else {
                // BGR
                row_ptr[x * 3 + 0] = c.b;
                row_ptr[x * 3 + 1] = c.g;
                row_ptr[x * 3 + 2] = c.r;
            }
        }
    }

    return out;
}

Result<bool> ImageExport::save_bmp(
    const FrameBuffer& buffer,
    const std::filesystem::path& file_path,
    bool include_alpha
) {
    if (file_path.has_parent_path()) {
        std::filesystem::create_directories(file_path.parent_path());
    }

    std::vector<uint8_t> encoded = encode_bmp(buffer, include_alpha);
    std::ofstream ofs(file_path, std::ios::binary);
    if (!ofs) {
        return Diagnostic{
            ErrorCode::EvaluationError,
            "Failed to open output file for writing BMP: " + file_path.string(),
            {}
        };
    }

    ofs.write(reinterpret_cast<const char*>(encoded.data()), static_cast<std::streamsize>(encoded.size()));
    if (!ofs) {
        return Diagnostic{
            ErrorCode::EvaluationError,
            "Failed to complete writing BMP file: " + file_path.string(),
            {}
        };
    }

    return true;
}

Result<bool> ImageExport::save_raw_rgba(
    const FrameBuffer& buffer,
    const std::filesystem::path& file_path
) {
    if (file_path.has_parent_path()) {
        std::filesystem::create_directories(file_path.parent_path());
    }

    std::vector<uint8_t> rgba;
    buffer.copy_to_rgba(rgba);

    std::ofstream ofs(file_path, std::ios::binary);
    if (!ofs) {
        return Diagnostic{
            ErrorCode::EvaluationError,
            "Failed to open output file for writing raw RGBA: " + file_path.string(),
            {}
        };
    }

    ofs.write(reinterpret_cast<const char*>(rgba.data()), static_cast<std::streamsize>(rgba.size()));
    return true;
}

Result<bool> ImageExport::save_ppm(
    const FrameBuffer& buffer,
    const std::filesystem::path& file_path
) {
    if (file_path.has_parent_path()) {
        std::filesystem::create_directories(file_path.parent_path());
    }

    std::ofstream ofs(file_path, std::ios::binary);
    if (!ofs) {
        return Diagnostic{
            ErrorCode::EvaluationError,
            "Failed to open output file for writing PPM: " + file_path.string(),
            {}
        };
    }

    const int w = buffer.width();
    const int h = buffer.height();
    ofs << "P6\n" << w << " " << h << "\n255\n";

    std::vector<uint8_t> row(static_cast<size_t>(w) * 3);
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            Color c = buffer.get_pixel(x, y);
            row[x * 3 + 0] = c.r;
            row[x * 3 + 1] = c.g;
            row[x * 3 + 2] = c.b;
        }
        ofs.write(reinterpret_cast<const char*>(row.data()), static_cast<std::streamsize>(row.size()));
    }

    return true;
}

} // namespace formulaic
