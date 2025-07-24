#pragma once

#include "command.hpp"
#include <string>

class DeleteCmd : public Command {
public:
    std::string name() const override {
        return "delete";
    }

    std::string desc() const override {
        return "cleanup";
    }

    int run(int argc, char *argv[]) override;
};
