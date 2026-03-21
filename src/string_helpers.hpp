#pragma once

// string_helpers.hpp — pure string/time utility functions with no git dependency.
// All functions are inline so this header is self-contained and unit-testable
// without linking against libgit2.

#include <chrono>
#include <format>
#include <string>
#include <string_view>

// ---------------------------------------------------------------------------
// strip_prefix
//
// If `s` starts with `prefix`, return the remainder.  Otherwise return `s`
// unchanged.
// ---------------------------------------------------------------------------
inline std::string strip_prefix(std::string_view s, std::string_view prefix) {
    if (s.substr(0, prefix.size()) == prefix)
        return std::string(s.substr(prefix.size()));
    return std::string(s);
}

// ---------------------------------------------------------------------------
// first_line
//
// Return the text up to (but not including) the first newline.
// Returns an empty string if `msg` is null.
// ---------------------------------------------------------------------------
inline std::string first_line(const char *msg) {
    if (!msg) return {};
    std::string_view sv(msg);
    auto pos = sv.find('\n');
    return std::string(pos == std::string_view::npos ? sv : sv.substr(0, pos));
}

// ---------------------------------------------------------------------------
// relative_time
//
// Format a point-in-time (seconds since epoch) as a human-readable relative
// string, e.g. "5 minutes ago".  Mirrors git's approximate relative-date
// output.
// ---------------------------------------------------------------------------
inline std::string relative_time(std::int64_t epoch_seconds) {
    using namespace std::chrono;
    auto commit_tp = system_clock::from_time_t(static_cast<time_t>(epoch_seconds));
    auto now       = system_clock::now();
    auto secs      = duration_cast<seconds>(now - commit_tp).count();
    if (secs < 0) secs = 0;

    if (secs < 90)
        return std::format("{} second{} ago", secs, secs == 1 ? "" : "s");

    auto mins = secs / 60;
    if (mins < 90)
        return std::format("{} minute{} ago", mins, mins == 1 ? "" : "s");

    auto hours = mins / 60;
    if (hours < 36)
        return std::format("{} hour{} ago", hours, hours == 1 ? "" : "s");

    auto days = hours / 24;
    if (days < 14)
        return std::format("{} day{} ago", days, days == 1 ? "" : "s");

    auto weeks = days / 7;
    if (weeks < 8)
        return std::format("{} week{} ago", weeks, weeks == 1 ? "" : "s");

    auto months = days / 30;
    if (months < 24)
        return std::format("{} month{} ago", months, months == 1 ? "" : "s");

    auto years = days / 365;
    return std::format("{} year{} ago", years, years == 1 ? "" : "s");
}
