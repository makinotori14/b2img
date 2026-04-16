#include "b2img/io/file_buffer.hpp"

#include <filesystem>
#include <fstream>
#include <stdexcept>

namespace b2img::io {

std::vector<std::uint8_t> FileBuffer::read_all(const std::string& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        throw std::runtime_error("unable to open input file: " + path);
    }

    const auto size = std::filesystem::file_size(path);
    std::vector<std::uint8_t> bytes(size);

    if (size > 0) {
        input.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
        if (!input) {
            throw std::runtime_error("unable to read input file: " + path);
        }
    }

    return bytes;
}

}  // namespace b2img::io
