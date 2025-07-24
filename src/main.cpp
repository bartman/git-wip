#include "command.hpp"
#include "cmd_config.hpp"
#include "cmd_delete.hpp"
#include "cmd_log.hpp"
#include "cmd_save.hpp"
#include "cmd_status.hpp"

#include <iostream>
#include <map>
#include <memory>
#include <vector>
#include "clipp.h"

void print_main_help(const std::vector<std::unique_ptr<Command>>& commands) {
    std::cout << "Manage Work In Progress\n\n";
    std::cout << "git-wip <command> [ --help | command options ]\n\n";
    for (const auto& cmd : commands) {
        std::cout << "    git-wip " << cmd->name() << "           # " << cmd->desc() << "\n";
    }
    std::cout << "\nUse git-wip <command> --help to see command options.\n";
}

int main(int argc, char *argv[]) {
    std::vector<std::unique_ptr<Command>> commands;
    commands.push_back(std::make_unique<ConfigCmd>());
    commands.push_back(std::make_unique<DeleteCmd>());
    commands.push_back(std::make_unique<LogCmd>());
    commands.push_back(std::make_unique<SaveCmd>());
    commands.push_back(std::make_unique<StatusCmd>());

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
        std::cerr << "Error: Unknown command '" << command_name << "'\n";
        print_main_help(commands);
        return 1;
    }

    return 0;
}
