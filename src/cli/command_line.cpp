#include "b2img/cli/command_line.hpp"
#include "b2img/core/encoding.hpp"

#include <charconv>
#include <stdexcept>
#include <string_view>

namespace b2img::cli {

namespace {

std::size_t parse_size(const std::string& value) {
    std::size_t result {};
    const auto* begin = value.data();
    const auto* end = value.data() + value.size();
    const auto parsed = std::from_chars(begin, end, result);

    if (parsed.ec != std::errc{} || parsed.ptr != end) {
        throw std::invalid_argument("invalid numeric value: " + value);
    }

    return result;
}

std::uint8_t parse_byte(const std::string& value) {
    const auto parsed = parse_size(value);
    if (parsed > 255U) {
        throw std::invalid_argument("numeric value is out of range: " + value);
    }

    return static_cast<std::uint8_t>(parsed);
}

b2img::core::EncodingSpec parse_encoding(std::string_view value) {
    const auto spec = b2img::core::find_encoding_spec(value);
    if (spec.has_value()) {
        return *spec;
    }

    throw std::invalid_argument("unknown encoding: " + std::string(value));
}

b2img::core::Layout parse_layout(std::string_view value) {
    using b2img::core::Layout;

    if (value == "linear") {
        return Layout::linear;
    }
    if (value == "planar-image") {
        return Layout::planar_image;
    }
    if (value == "planar-row") {
        return Layout::planar_row;
    }
    if (value == "planar-pixel") {
        return Layout::planar_pixel;
    }

    throw std::invalid_argument("unknown layout: " + std::string(value));
}

b2img::core::PaletteKind parse_palette(std::string_view value) {
    using b2img::core::PaletteKind;

    if (value == "grayscale") {
        return PaletteKind::grayscale;
    }
    if (value == "vga16") {
        return PaletteKind::vga16;
    }
    if (value == "cga0") {
        return PaletteKind::cga0;
    }
    if (value == "cga1") {
        return PaletteKind::cga1;
    }
    if (value == "cga2") {
        return PaletteKind::cga2;
    }

    throw std::invalid_argument("unknown palette: " + std::string(value));
}

b2img::core::BitOrder parse_bit_order(std::string_view value) {
    using b2img::core::BitOrder;

    if (value == "msb") {
        return BitOrder::msb_first;
    }
    if (value == "lsb") {
        return BitOrder::lsb_first;
    }

    throw std::invalid_argument("unknown bit order: " + std::string(value));
}

}  // namespace

CommandLine CommandLineParser::parse(const std::vector<std::string>& args) {
    if (args.empty()) {
        return CommandLine{};
    }

    CommandLine command_line;
    const auto& command = args.front();

    if (command == "--help" || command == "-h" || command == "help") {
        return command_line;
    }
    if (command == "inspect") {
        command_line.command = CommandKind::inspect;
    } else if (command == "render") {
        command_line.command = CommandKind::render;
    } else {
        throw std::invalid_argument("unknown command: " + command);
    }

    for (std::size_t index = 1; index < args.size(); ++index) {
        const auto& key = args[index];
        std::optional<std::string> value;

        if (key.rfind("--", 0) != 0) {
            throw std::invalid_argument("expected an option starting with --, got: " + key);
        }

        if (key == "--flip-vertical") {
            apply_option(command_line, key, std::nullopt);
            continue;
        }

        if (index + 1 >= args.size()) {
            throw std::invalid_argument("missing value for option: " + key);
        }

        value = args[++index];
        apply_option(command_line, key, value);
    }

    return command_line;
}

void CommandLineParser::apply_option(CommandLine& command_line, const std::string& key, const std::optional<std::string>& value) {
    auto require_value = [&]() -> const std::string& {
        if (!value.has_value()) {
            throw std::invalid_argument("missing value for option: " + key);
        }
        return *value;
    };

    auto& request = command_line.request;

    if (key == "--input") {
        request.input_path = require_value();
        return;
    }
    if (key == "--output") {
        request.output_path = require_value();
        return;
    }
    if (key == "--width") {
        request.width = parse_size(require_value());
        return;
    }
    if (key == "--height") {
        request.height = parse_size(require_value());
        return;
    }
    if (key == "--bit-offset") {
        request.bit_offset = parse_size(require_value());
        return;
    }
    if (key == "--bits-per-pixel") {
        request.bits_per_pixel = parse_byte(require_value());
        return;
    }
    if (key == "--encoding") {
        const auto encoding = parse_encoding(require_value());
        request.encoding_kind = encoding.kind;
        request.encoding_name = std::string(encoding.name);
        return;
    }
    if (key == "--layout") {
        request.layout = parse_layout(require_value());
        return;
    }
    if (key == "--palette") {
        request.palette = parse_palette(require_value());
        return;
    }
    if (key == "--bit-order") {
        request.bit_order = parse_bit_order(require_value());
        return;
    }
    if (key == "--flip-vertical") {
        request.flip_vertical = true;
        return;
    }

    throw std::invalid_argument("unknown option: " + key);
}

std::string CommandLineParser::help_text() {
    return std::string{}
        + "b2img\n"
        "  Explore binary data as image slices.\n"
        "\n"
        "Commands:\n"
        "  help\n"
        "  inspect --input <file> --width <n> --height <n> [options]\n"
        "  render  --input <file> --output <file.ppm> --width <n> --height <n> [options]\n"
        "\n"
        "Options:\n"
        + "  --encoding " + b2img::core::encoding_names_joined() + "\n"
        "                                       Default: indexed\n"
        "  --layout linear|planar-image|planar-row|planar-pixel\n"
        "                                       Default: linear\n"
        "  --bits-per-pixel <n>                 Default: 8\n"
        "  --bit-offset <n>                     Default: 0\n"
        "  --palette grayscale|vga16|cga0|cga1|cga2\n"
        "                                       Default: grayscale\n"
        "  --bit-order msb|lsb                  Default: msb\n"
        "  --flip-vertical\n"
        "\n"
        "Examples:\n"
        "  b2img inspect --input data.bin --width 128 --height 128 --bits-per-pixel 4 --palette vga16\n"
        "  b2img render --input data.bin --output out.ppm --width 320 --height 200 --encoding abgr32\n";
}

}  // namespace b2img::cli
