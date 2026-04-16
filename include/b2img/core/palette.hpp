#pragma once

#include "b2img/core/image.hpp"
#include "b2img/core/render_request.hpp"

#include <cstddef>
#include <vector>

namespace b2img::core {

class Palette {
public:
    [[nodiscard]] static std::vector<Rgb> make(PaletteKind kind, std::size_t color_count);
};

}  // namespace b2img::core
