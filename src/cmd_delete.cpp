#include "cmd_delete.hpp"
#include "clipp.h"
#include <print>
#include <format>
#include <sstream> // Required for std::ostringstream
#include <vector>
#include <string>

int DeleteCmd::run(int argc, char *argv[]) {
    std::vector<std::string> files;

    clipp::group cli(clipp::values("files", files).doc("Files to delete"));

    // Check for --help
    if (argc > 1 && std::string(argv[1]) == "--help") {
        std::ostringstream oss;
        oss << clipp::make_man_page(cli, this->name());
        std::println("{}", oss.str());
        return 0;
    }

    if (!clipp::parse(argc, argv, cli)) {
        std::ostringstream oss;
        oss << clipp::make_man_page(cli, this->name());
        std::println("{}", oss.str());
        return 1;
    }

    std::println("Delete command executed.");
    if (!files.empty()) {
        std::print("Files: ");
        for (const auto& file : files) {
            std::print("{} ", file);
        }
        std::println("");
    }
    return 0;
}

