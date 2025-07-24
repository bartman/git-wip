#pragma once

#include "command.hpp"
#include <string>

class ConfigCmd : public Command {
public:
    std::string name() const override {
        return "config";
    }

    std::string desc() const override {
        return "configure git-wip";
    }

    int run(int argc, char *argv[]) override;
};
