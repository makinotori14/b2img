#pragma once

#include <string>
#include <vector>

namespace b2img::app {

class Application {
public:
    int run(const std::vector<std::string>& args) const;
};

}  // namespace b2img::app
