#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace b2img::io {

class FileBuffer {
public:
    [[nodiscard]] static std::vector<std::uint8_t> read_all(const std::string& path);
};

}  // namespace b2img::io
