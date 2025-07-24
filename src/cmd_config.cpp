#include "cmd_config.hpp"
#include "clipp.h"
#include <iostream>
#include <print>
#include <format>
#include <sstream> // Required for std::ostringstream
#include <string>
#include <map>

// Dummy configuration storage for demonstration
static std::map<std::string, std::string> s_config_store = {
    {"user.name", "John Doe"},
    {"user.email", "john.doe@example.com"},
    {"core.editor", "vim"}
};

int ConfigCmd::run(int argc, char *argv[]) {
    std::string var_name;
    std::string var_value;
    bool set_value = false;

    if (argc == 1) {
        // List all config variables if only the command name is present
        for (const auto& pair : s_config_store) {
            std::println("{}={}", pair.first, pair.second);
        }
        return 0;
    }

    // Check for --help
    if (argc > 1 && std::string(argv[1]) == "--help") {
        clipp::group cli(
            clipp::values("var", var_name).doc("Configuration variable name"),
            clipp::opt_values("var=val", var_value).doc("Set configuration variable")
        );
        std::ostringstream oss;
        oss << clipp::make_man_page(cli, this->name());
        std::println("{}", oss.str());
        return 0;
    }

    // Manual parsing of arguments
    if (argc == 2) { // git-wip config var or git-wip config var=val
        std::string input_arg = argv[1];
        size_t eq_pos = input_arg.find('=');
        if (eq_pos != std::string::npos) {
            var_name = input_arg.substr(0, eq_pos);
            var_value = input_arg.substr(eq_pos + 1);
            set_value = true;
        } else {
            var_name = input_arg;
            set_value = false; // Explicitly set to false for get operations
        }
    } else {
        // Invalid number of arguments for config command
        std::println(std::cerr, "Error: Invalid number of arguments for config command.\n");
        return 1;
    }

    if (set_value) {
        s_config_store[var_name] = var_value;
        std::println("Set {} to {}", var_name, var_value);
    } else { // This is a get operation
        auto it = s_config_store.find(var_name);
        if (it != s_config_store.end()) {
            std::println("{}", it->second);
        } else {
            std::println(std::cerr, "Error: Unknown configuration variable '{}'\n", var_name);
        }
    }

    return 0;
}
