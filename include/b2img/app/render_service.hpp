#pragma once

#include "b2img/core/analyzer.hpp"
#include "b2img/core/image.hpp"
#include "b2img/core/render_request.hpp"

#include <string>

namespace b2img::app {

struct InspectionResult {
    std::size_t file_size_bytes {};
    b2img::core::AnalysisReport analysis;
};

struct RenderResult {
    InspectionResult inspection;
    b2img::core::Image image;
};

class RenderService {
public:
    [[nodiscard]] InspectionResult inspect(const b2img::core::RenderRequest& request) const;
    [[nodiscard]] RenderResult render(const b2img::core::RenderRequest& request) const;
    void save_ppm(const std::string& output_path, const b2img::core::Image& image) const;

    static void validate_request(const b2img::core::RenderRequest& request, bool require_output_path);
};

}  // namespace b2img::app
