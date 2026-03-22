#include "color.hpp"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <format>

#include <unistd.h>

bool g_wip_color = true;

namespace {

std::string lower_copy(std::string s) {
    std::ranges::transform(s, s.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return s;
}

bool stdout_is_tty() {
    return isatty(fileno(stdout)) == 1;
}

} // namespace

void color_init() {
    // safe fallback
    g_wip_color = false;

    const char *env = std::getenv("WIP_COLOR");
    if (env == nullptr) {
        g_wip_color = stdout_is_tty();
        return;
    }

    std::string mode = lower_copy(env);
    if (mode == "1" || mode == "on" || mode == "always") {
        g_wip_color = true;
        return;
    }

    if (mode == "0" || mode == "off" || mode == "never") {
        g_wip_color = false;
        return;
    }

    if (mode.empty() || mode == "auto") {
        g_wip_color = stdout_is_tty();
        return;
    }
}

std::string Color::red() {
    return g_wip_color ? "\x1b[31m" : "";
}

std::string Color::green() {
    return g_wip_color ? "\x1b[32m" : "";
}

std::string Color::yellow() {
    return g_wip_color ? "\x1b[33m" : "";
}

std::string Color::reset() {
    return g_wip_color ? "\x1b[0m" : "";
}

std::string Color::rgb(int r, int g, int b) {
    r = std::clamp(r, 0, 255);
    g = std::clamp(g, 0, 255);
    b = std::clamp(b, 0, 255);
    return g_wip_color ? std::format("\x1b[38;2;{};{};{}m", r, g, b) : "";
}

std::string color_branch(std::string_view branch_name) {
    return Color::green() + std::string(branch_name) + Color::reset();
}

std::string color_wip_branch(std::string_view wip_branch_name) {
    return Color::red() + std::string(wip_branch_name) + Color::reset();
}

std::string color_commit_hash(std::string_view commit_hash) {
    return Color::yellow() + std::string(commit_hash) + Color::reset();
}
