#pragma once

#include "b2img/core/encoding.hpp"

#include <cstddef>
#include <cstdint>
#include <string>

namespace b2img::core {

enum class Layout {
    linear,
    planar_image,
    planar_row,
    planar_pixel
};

enum class PaletteKind {
    grayscale,
    vga16,
    cga0,
    cga1,
    cga2
};

enum class BitOrder {
    msb_first,
    lsb_first
};

struct RenderRequest {
    std::string input_path;
    std::string output_path;
    std::size_t width {};
    std::size_t height {};
    std::size_t bit_offset {};
    std::uint8_t bits_per_pixel {8};
    EncodingKind encoding_kind {EncodingKind::indexed};
    std::string encoding_name {"indexed"};
    Layout layout {Layout::linear};
    PaletteKind palette {PaletteKind::grayscale};
    BitOrder bit_order {BitOrder::msb_first};
    bool flip_vertical {false};
    int rotation_turns {};
};

}  // namespace b2img::core
