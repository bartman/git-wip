#include "command.hpp"
#include "cmd_delete.hpp"
#include "cmd_log.hpp"
#include "cmd_save.hpp"
#include "cmd_status.hpp"

#include <cstdlib>
#include <iostream>
#include <print>
#include <format>
#include <sstream> // Required for std::ostringstream
#include <map>
#include <memory>
#include <vector>
#include "clipp.h"
#include "spdlog/spdlog.h"

void print_main_help(const std::vector<std::unique_ptr<Command>>& commands, std::ostream &os = std::cout) {
    std::println(os, "Manage Work In Progress\n");
    std::println(os, "git-wip <command> [ --help | command options ]\n");
    for (const auto& cmd : commands) {
        std::println("    git-wip {:20} # {}", cmd->name(), cmd->desc());
    }
    std::println(os, "\nUse git-wip <command> --help to see command options.\n");
}

int main(int argc, char *argv[]) {
    // Check WIP_DEBUG environment variable for debug logging
    const char* wip_debug = std::getenv("WIP_DEBUG");
    if (wip_debug != nullptr && wip_debug[0] != '\0' && wip_debug[0] != '0') {
        spdlog::set_level(spdlog::level::debug);
        spdlog::debug("Debug logging enabled via WIP_DEBUG environment variable");
    }

    std::vector<std::unique_ptr<Command>> commands;
    commands.push_back(std::make_unique<StatusCmd>());
    commands.push_back(std::make_unique<LogCmd>());
    commands.push_back(std::make_unique<SaveCmd>());
    commands.push_back(std::make_unique<DeleteCmd>());

    std::map<std::string, Command*> command_map;
    for (const auto& cmd : commands) {
        command_map[cmd->name()] = cmd.get();
    }

    if (argc < 2) {
        print_main_help(commands);
        return 0;
    }

    std::string command_name = argv[1];

    if (command_name == "help" || command_name == "--help" || command_name == "-h") {
        print_main_help(commands);
        return 0;
    }

    auto it = command_map.find(command_name);
    if (it != command_map.end()) {
        Command* cmd = it->second;
        // Pass remaining arguments to the command, skipping argv[0] (program name),
        // so that argv[1] (command name) becomes argv[0] inside the command parser
        return cmd->run(argc - 1, argv + 1);
    } else {
        spdlog::error("Error: Unknown command '{}'\n", command_name);
        print_main_help(commands, std::cerr);
        return 1;
    }

    return 0;
}
