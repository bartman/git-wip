#include "cmd_log.hpp"
#include "clipp.h"
#include <print>
#include <format>
#include <sstream> // Required for std::ostringstream
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

    std::println("Log command executed. Pretty: {}", pretty ? "true" : "false");
    if (!files.empty()) {
        std::print("Files: ");
        for (const auto& file : files) {
            std::print("{} ", file);
        }
        std::println("");
    }
    return 0;
}
