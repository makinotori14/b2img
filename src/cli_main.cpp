#include "b2img/app/application.hpp"

#include <string>
#include <vector>

int main(int argc, char** argv) {
    std::vector<std::string> args;
    args.reserve(argc > 0 ? static_cast<std::size_t>(argc - 1) : 0U);

    for (int index = 1; index < argc; ++index) {
        args.emplace_back(argv[index]);
    }

    const b2img::app::Application application;
    return application.run(args);
}
