#include "b2img/app/render_service.hpp"

#include "b2img/core/encoding.hpp"
#include "b2img/core/decoder.hpp"
#include "b2img/io/file_buffer.hpp"
#include "b2img/io/ppm_writer.hpp"

#include <stdexcept>

namespace b2img::app {

InspectionResult RenderService::inspect(const b2img::core::RenderRequest& request) const {
    validate_request(request, false);

    const auto bytes = b2img::io::FileBuffer::read_all(request.input_path);
    return InspectionResult{
        .file_size_bytes = bytes.size(),
        .analysis = b2img::core::Analyzer::analyze(bytes.size(), request),
    };
}

RenderResult RenderService::render(const b2img::core::RenderRequest& request) const {
    validate_request(request, false);

    const auto bytes = b2img::io::FileBuffer::read_all(request.input_path);

    return RenderResult{
        .inspection = InspectionResult{
            .file_size_bytes = bytes.size(),
            .analysis = b2img::core::Analyzer::analyze(bytes.size(), request),
        },
        .image = b2img::core::Decoder::decode(bytes, request),
    };
}

void RenderService::save_ppm(const std::string& output_path, const b2img::core::Image& image) const {
    if (output_path.empty()) {
        throw std::invalid_argument("missing required output path");
    }

    b2img::io::PpmWriter::write(output_path, image);
}

void RenderService::validate_request(const b2img::core::RenderRequest& request, bool require_output_path) {
    if (request.input_path.empty()) {
        throw std::invalid_argument("missing required option --input");
    }
    if (request.width == 0 || request.height == 0) {
        throw std::invalid_argument("width and height must be greater than zero");
    }
    const auto encoding = b2img::core::find_encoding_spec(request.encoding_name);
    if (!encoding.has_value()) {
        throw std::invalid_argument("unsupported encoding: " + request.encoding_name);
    }
    if (encoding->kind != request.encoding_kind) {
        throw std::invalid_argument("encoding metadata is inconsistent");
    }
    if (require_output_path && request.output_path.empty()) {
        throw std::invalid_argument("missing required output path");
    }
    if (request.encoding_kind != b2img::core::EncodingKind::indexed &&
        request.layout != b2img::core::Layout::linear) {
        throw std::invalid_argument("non-indexed encodings currently support only linear layout");
    }
}

}  // namespace b2img::app
