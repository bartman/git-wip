#include "cmd_status.hpp"
#include "clipp.h"
#include <print>
#include <format>

int StatusCmd::run(int argc, char *argv[]) {
    // Check for --help
    if (argc > 1 && std::string(argv[1]) == "--help") {
        std::println("SYNOPSIS");
        std::println("        status\n");
        std::println("Inspect changes in the work in progress.");
        return 0;
    }

    std::println("Status command executed. (No options yet)");
    return 0;
}
