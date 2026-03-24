#pragma once

#include "command.hpp"
#include <string>

class StatusCmd : public Command {
public:
    std::string name() const override {
        return "status";
    }

    std::string desc() const override {
        return "inspect changes";
    }

    int run(int argc, char *argv[]) override;
};
