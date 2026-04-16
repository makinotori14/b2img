#pragma once

#include "b2img/core/image.hpp"

#include <string>

namespace b2img::io {

class PpmWriter {
public:
    static void write(const std::string& path, const b2img::core::Image& image);
};

}  // namespace b2img::io
