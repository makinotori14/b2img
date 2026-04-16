#pragma once

#include "b2img/core/image.hpp"
#include "b2img/core/render_request.hpp"

#include <cstdint>
#include <vector>

namespace b2img::core {

class Decoder {
public:
    [[nodiscard]] static Image decode(const std::vector<std::uint8_t>& bytes, const RenderRequest& request);
};

}  // namespace b2img::core
