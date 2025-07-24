#include "cmd_status.hpp"
#include "clipp.h"
#include <iostream>

int StatusCmd::run(int argc, char *argv[]) {
    // Check for --help
    if (argc > 1 && std::string(argv[1]) == "--help") {
        std::cout << "SYNOPSIS\n";
        std::cout << "        status\n\n";
        std::cout << "Inspect changes in the work in progress.\n";
        return 0;
    }

    std::cout << "Status command executed. (No options yet)\n";
    return 0;
}
