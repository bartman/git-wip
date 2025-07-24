#include "cmd_config.hpp"
#include "clipp.h"
#include <iostream>
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
            std::cout << pair.first << "=" << pair.second << "\n";
        }
        return 0;
    }

    // Check for --help
    if (argc > 1 && std::string(argv[1]) == "--help") {
        clipp::group cli(
            clipp::values("var", var_name).doc("Configuration variable name"),
            clipp::option("var=val").call([&](const std::string& s) {
                size_t eq_pos = s.find('=');
                if (eq_pos != std::string::npos) {
                    var_name = s.substr(0, eq_pos);
                    var_value = s.substr(eq_pos + 1);
                    set_value = true;
                }
            }).doc("Set configuration variable")
        );
        std::cout << clipp::make_man_page(cli, this->name());
        return 0;
    }


    clipp::group cli(
        clipp::values("var", var_name).doc("Configuration variable name"),
        clipp::option("var=val").call([&](const std::string& s) {
            size_t eq_pos = s.find('=');
            if (eq_pos != std::string::npos) {
                var_name = s.substr(0, eq_pos);
                var_value = s.substr(eq_pos + 1);
                set_value = true;
            }
        }).doc("Set configuration variable")
    );

    if (!clipp::parse(argc, argv, cli)) {
        std::cout << clipp::make_man_page(cli, this->name());
        return 1;
    }

    if (set_value) {
        s_config_store[var_name] = var_value;
        std::cout << "Set " << var_name << " to " << var_value << "\n";
    } else if (!var_name.empty()) {
        auto it = s_config_store.find(var_name);
        if (it != s_config_store.end()) {
            std::cout << it->second << "\n";
        } else {
            std::cerr << "Error: Unknown configuration variable '" << var_name << "'\n";
            return 1;
        }
    } else {
        // List all config variables
        for (const auto& pair : s_config_store) {
            std::cout << pair.first << "=" << pair.second << "\n";
        }
    }

    return 0;
}
