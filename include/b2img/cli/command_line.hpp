#pragma once

#include "b2img/core/render_request.hpp"

#include <optional>
#include <string>
#include <vector>

namespace b2img::cli {

enum class CommandKind {
    help,
    inspect,
    render
};

struct CommandLine {
    CommandKind command {CommandKind::help};
    b2img::core::RenderRequest request;
};

class CommandLineParser {
public:
    [[nodiscard]] static CommandLine parse(const std::vector<std::string>& args);
    [[nodiscard]] static std::string help_text();

private:
    static void apply_option(CommandLine& command_line, const std::string& key, const std::optional<std::string>& value);
};

}  // namespace b2img::cli
