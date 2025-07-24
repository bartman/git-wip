#pragma once

#include "command.hpp"
#include <string>

class LogCmd : public Command {
public:
    std::string name() const override {
        return "log";
    }

    std::string desc() const override {
        return "look at WIP history";
    }

    int run(int argc, char *argv[]) override;
};
