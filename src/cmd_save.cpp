#include "cmd_save.hpp"
#include "clipp.h"
#include <print>
#include <format>
#include <sstream> // Required for std::ostringstream
#include <vector>
#include <string>

int SaveCmd::run(int argc, char *argv[]) {
    bool editor = false;
    bool untracked = false;
    std::string message;
    std::vector<std::string> files;

    clipp::group cli(
        clipp::option("--editor").set(editor).doc("Use editor to compose message"),
        clipp::option("--untracked").set(untracked).doc("Include untracked files"),
        clipp::option("--message").doc("Commit message").call([&](const std::string& msg){ message = msg; }),
        clipp::values("files", files).doc("Files to save")
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

    std::println("Save command executed. Editor: {}, Untracked: {}, Message: {}",
              editor ? "true" : "false", untracked ? "true" : "false", message);
    if (!files.empty()) {
        std::print("Files: ");
        for (const auto& file : files) {
            std::print("{} ", file);
        }
        std::println("");
    }
    return 0;
}
