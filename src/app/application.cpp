#include "b2img/app/application.hpp"

#include "b2img/app/render_service.hpp"
#include "b2img/cli/command_line.hpp"
#include "b2img/core/encoding.hpp"
#include "b2img/core/render_request.hpp"

#include <iostream>
#include <stdexcept>

namespace b2img::app {

namespace {

std::string encoding_name(const b2img::core::RenderRequest& request) {
    return request.encoding_name;
}

std::string layout_name(b2img::core::Layout layout) {
    using b2img::core::Layout;

    switch (layout) {
        case Layout::linear:
            return "linear";
        case Layout::planar_image:
            return "planar-image";
        case Layout::planar_row:
            return "planar-row";
        case Layout::planar_pixel:
            return "planar-pixel";
    }

    return "unknown";
}

std::string palette_name(b2img::core::PaletteKind palette) {
    using b2img::core::PaletteKind;

    switch (palette) {
        case PaletteKind::grayscale:
            return "grayscale";
        case PaletteKind::vga16:
            return "vga16";
        case PaletteKind::cga0:
            return "cga0";
        case PaletteKind::cga1:
            return "cga1";
        case PaletteKind::cga2:
            return "cga2";
    }

    return "unknown";
}

void print_report(const b2img::app::InspectionResult& inspection, const b2img::core::RenderRequest& request) {
    std::cout
        << "input: " << request.input_path << '\n'
        << "file-size-bytes: " << inspection.file_size_bytes << '\n'
        << "width: " << request.width << '\n'
        << "height: " << request.height << '\n'
        << "encoding: " << encoding_name(request) << '\n'
        << "layout: " << layout_name(request.layout) << '\n'
        << "bits-per-pixel: " << static_cast<unsigned>(request.bits_per_pixel) << '\n'
        << "bit-offset: " << request.bit_offset << '\n'
        << "palette: " << palette_name(request.palette) << '\n'
        << "required-bits: " << inspection.analysis.required_bits << '\n'
        << "available-bits: " << inspection.analysis.available_bits << '\n'
        << "trailing-bits: " << inspection.analysis.trailing_bits << '\n'
        << "enough-data: " << (inspection.analysis.enough_data ? "yes" : "no") << '\n';
}

}  // namespace

int Application::run(const std::vector<std::string>& args) const {
    try {
        const auto command_line = b2img::cli::CommandLineParser::parse(args);
        const RenderService service;

        if (command_line.command == b2img::cli::CommandKind::help) {
            std::cout << b2img::cli::CommandLineParser::help_text();
            return 0;
        }

        if (command_line.command == b2img::cli::CommandKind::inspect) {
            print_report(service.inspect(command_line.request), command_line.request);
            return 0;
        }

        const auto result = service.render(command_line.request);
        service.save_ppm(command_line.request.output_path, result.image);

        std::cout
            << "rendered " << result.image.width() << "x" << result.image.height()
            << " image to " << command_line.request.output_path << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "error: " << error.what() << '\n';
        return 1;
    }
}

}  // namespace b2img::app
