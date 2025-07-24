#include "cmd_save.hpp"
#include "clipp.h"
#include <iostream>
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
        std::cout << clipp::make_man_page(cli, this->name());
        return 0;
    }

    if (!clipp::parse(argc, argv, cli)) {
        std::cout << clipp::make_man_page(cli, this->name());
        return 1;
    }

    std::cout << "Save command executed. Editor: " << (editor ? "true" : "false")
              << ", Untracked: " << (untracked ? "true" : "false")
              << ", Message: \"" << message << "\"\n";
    if (!files.empty()) {
        std::cout << "Files: ";
        for (const auto& file : files) {
            std::cout << file << " ";
        }
        std::cout << "\n";
    }
    return 0;
}
