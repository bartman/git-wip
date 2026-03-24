#pragma once

#include <string>
#include <string_view>

extern bool g_wip_color;

void color_init();

class Color {
public:
    static std::string red();
    static std::string green();
    static std::string yellow();
    static std::string reset();
    static std::string rgb(int r, int g, int b);
};

std::string color_branch(std::string_view branch_name);
std::string color_wip_branch(std::string_view wip_branch_name);
std::string color_commit_hash(std::string_view commit_hash);
