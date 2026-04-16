#include "b2img/core/palette.hpp"

#include <algorithm>
#include <stdexcept>

namespace b2img::core {

namespace {

std::vector<Rgb> grayscale(std::size_t color_count) {
    std::vector<Rgb> colors;
    colors.reserve(color_count);

    if (color_count == 0) {
        return colors;
    }

    const auto max_index = color_count - 1U;
    for (std::size_t index = 0; index < color_count; ++index) {
        const auto value = static_cast<std::uint8_t>(max_index == 0 ? 0 : (index * 255U) / max_index);
        colors.push_back(Rgb{value, value, value});
    }

    return colors;
}

std::vector<Rgb> fixed_palette(const std::vector<Rgb>& base, std::size_t color_count) {
    if (color_count > base.size()) {
        throw std::invalid_argument("requested palette contains more colors than the selected preset supports");
    }

    return std::vector<Rgb>(base.begin(), base.begin() + static_cast<std::ptrdiff_t>(color_count));
}

}  // namespace

std::vector<Rgb> Palette::make(PaletteKind kind, std::size_t color_count) {
    static const std::vector<Rgb> vga16 = {
        {0x00, 0x00, 0x00}, {0x00, 0x00, 0xAA}, {0x00, 0xAA, 0x00}, {0x00, 0xAA, 0xAA},
        {0xAA, 0x00, 0x00}, {0xAA, 0x00, 0xAA}, {0xAA, 0x55, 0x00}, {0xAA, 0xAA, 0xAA},
        {0x55, 0x55, 0x55}, {0x55, 0x55, 0xFF}, {0x55, 0xFF, 0x55}, {0x55, 0xFF, 0xFF},
        {0xFF, 0x55, 0x55}, {0xFF, 0x55, 0xFF}, {0xFF, 0xFF, 0x55}, {0xFF, 0xFF, 0xFF},
    };

    static const std::vector<Rgb> cga0 = {
        {0x00, 0x00, 0x00}, {0x00, 0xAA, 0x00}, {0xAA, 0x00, 0x00}, {0xAA, 0x55, 0x00},
    };

    static const std::vector<Rgb> cga1 = {
        {0x00, 0x00, 0x00}, {0x00, 0xAA, 0xAA}, {0xAA, 0x00, 0xAA}, {0xAA, 0xAA, 0xAA},
    };

    static const std::vector<Rgb> cga2 = {
        {0x00, 0x00, 0x00}, {0x00, 0xAA, 0xAA}, {0xAA, 0x00, 0x00}, {0xAA, 0xAA, 0xAA},
    };

    switch (kind) {
        case PaletteKind::grayscale:
            return grayscale(color_count);

        case PaletteKind::vga16:
            return fixed_palette(vga16, color_count);

        case PaletteKind::cga0:
            return fixed_palette(cga0, color_count);

        case PaletteKind::cga1:
            return fixed_palette(cga1, color_count);

        case PaletteKind::cga2:
            return fixed_palette(cga2, color_count);
    }

    throw std::invalid_argument("unsupported palette kind");
}

}  // namespace b2img::core
