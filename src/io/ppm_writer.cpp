#include "b2img/io/ppm_writer.hpp"

#include <fstream>
#include <stdexcept>

namespace b2img::io {

void PpmWriter::write(const std::string& path, const b2img::core::Image& image) {
    if (image.empty()) {
        throw std::invalid_argument("cannot write an empty image");
    }

    std::ofstream output(path, std::ios::binary);
    if (!output) {
        throw std::runtime_error("unable to open output file: " + path);
    }

    output << "P6\n" << image.width() << ' ' << image.height() << "\n255\n";
    for (const auto& pixel : image.pixels()) {
        output.put(static_cast<char>(pixel.red));
        output.put(static_cast<char>(pixel.green));
        output.put(static_cast<char>(pixel.blue));
    }

    if (!output) {
        throw std::runtime_error("unable to write output file: " + path);
    }
}

}  // namespace b2img::io
