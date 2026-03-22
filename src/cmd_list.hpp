#pragma once

#include "command.hpp"

#include <string>

class ListCmd : public Command {
public:
    std::string name() const override {
        return "list";
    }

    std::string desc() const override {
        return "list wip branches";
    }

    int run(int argc, char *argv[]) override;
};
