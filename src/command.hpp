#pragma once

#include <string>
#include <vector>

class Command {
public:
    // Pure virtual methods to be implemented by derived classes
    virtual std::string name() const = 0;
    virtual std::string desc() const = 0;
    virtual int run(int argc, char *argv[]) = 0;

    // Virtual destructor to ensure proper cleanup of derived classes
    virtual ~Command() = default;
};
