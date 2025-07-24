#pragma once

#include "command.hpp"
#include <string>

class SaveCmd : public Command {
public:
    std::string name() const override {
        return "save";
    }

    std::string desc() const override {
        return "save current work";
    }

    int run(int argc, char *argv[]) override;
};
