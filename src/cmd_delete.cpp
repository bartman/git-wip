#include "cmd_delete.hpp"
#include "clipp.h"
#include <iostream>
#include <vector>
#include <string>

int DeleteCmd::run(int argc, char *argv[]) {
    std::vector<std::string> files;

    clipp::group cli(clipp::values("files", files).doc("Files to delete"));

    // Check for --help
    if (argc > 1 && std::string(argv[1]) == "--help") {
        std::cout << clipp::make_man_page(cli, this->name());
        return 0;
    }

    if (!clipp::parse(argc, argv, cli)) {
        std::cout << clipp::make_man_page(cli, this->name());
        return 1;
    }

    std::cout << "Delete command executed.\n";
    if (!files.empty()) {
        std::cout << "Files: ";
        for (const auto& file : files) {
            std::cout << file << " ";
        }
        std::cout << "\n";
    }
    return 0;
}

