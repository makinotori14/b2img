#include "b2img/core/analyzer.hpp"

#include "b2img/core/encoding.hpp"

#include <stdexcept>

namespace b2img::core {

namespace {

std::size_t required_bits_for(const RenderRequest& request) {
    if (request.width == 0 || request.height == 0) {
        throw std::invalid_argument("width and height must be greater than zero");
    }

    const std::size_t pixel_count = request.width * request.height;

    switch (request.encoding_kind) {
        case EncodingKind::indexed:
            if (request.layout == Layout::linear) {
                if (request.bits_per_pixel == 0 || request.bits_per_pixel > 8) {
                    throw std::invalid_argument("indexed mode requires bits-per-pixel in range 1..8");
                }

                return pixel_count * request.bits_per_pixel;
            }

            if (request.bits_per_pixel != 4) {
                throw std::invalid_argument("planar indexed layouts require bits-per-pixel equal to 4");
            }

            return pixel_count * 4;

        case EncodingKind::packed24:
            return pixel_count * 24;

        case EncodingKind::packed32:
            return pixel_count * 32;
    }

    throw std::invalid_argument("unsupported encoding");
}

}  // namespace

AnalysisReport Analyzer::analyze(std::size_t file_size_bytes, const RenderRequest& request) {
    const std::size_t file_bits = file_size_bytes * 8;
    const std::size_t required_bits = required_bits_for(request);
    const std::size_t available_bits = request.bit_offset >= file_bits ? 0U : (file_bits - request.bit_offset);
    const bool enough = available_bits >= required_bits;

    return AnalysisReport{
        .required_bits = required_bits,
        .available_bits = available_bits,
        .trailing_bits = enough ? available_bits - required_bits : 0,
        .enough_data = enough,
    };
}

}  // namespace b2img::core
