#pragma once

#include "b2img/core/render_request.hpp"

#include <cstddef>

namespace b2img::core {

struct AnalysisReport {
    std::size_t required_bits {};
    std::size_t available_bits {};
    std::size_t trailing_bits {};
    bool enough_data {false};
};

class Analyzer {
public:
    [[nodiscard]] static AnalysisReport analyze(std::size_t file_size_bytes, const RenderRequest& request);
};

}  // namespace b2img::core
