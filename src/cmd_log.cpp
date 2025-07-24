#include "cmd_log.hpp"
#include "clipp.h"
#include <iostream>
#include <vector>
#include <string>

int LogCmd::run(int argc, char *argv[]) {
    bool pretty = false;
    std::vector<std::string> files;

    clipp::group cli(
        clipp::option("--pretty").set(pretty).doc("Pretty print log"),
        clipp::values("files", files).doc("Files to log")
    );

    // Check for --help
    if (argc > 1 && std::string(argv[1]) == "--help") {
        std::cout << clipp::make_man_page(cli, this->name());
        return 0;
    }

    if (!clipp::parse(argc, argv, cli)) {
        std::cout << clipp::make_man_page(cli, this->name());
        return 1;
    }

    std::cout << "Log command executed. Pretty: " << (pretty ? "true" : "false") << "\n";
    if (!files.empty()) {
        std::cout << "Files: ";
        for (const auto& file : files) {
            std::cout << file << " ";
        }
        std::cout << "\n";
    }
    return 0;
}
