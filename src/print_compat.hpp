#pragma once

// print_compat.hpp — portable std::print / std::println
//
// C++23's <print> (P2093) is not available on all compilers we support:
//   - GCC 14+   ✓
//   - GCC 13    ✗  (Ubuntu 24.04)
//   - GCC 12    ✗  (Debian stable)
//   - Clang 17+ ✓  (with libc++)
//
// When WIP_HAVE_STD_PRINT is defined by CMake (via check_cxx_source_compiles),
// we use the real <print>.  Otherwise we shim std::print / std::println on top
// of {fmt}, which is fetched by FetchContent and has an identical API.

#ifdef WIP_HAVE_STD_PRINT

#include <print>

#else // fallback: map std::print / std::println → fmt::print / fmt::println

#include <fmt/core.h>
#include <fmt/ostream.h>

#include <ostream>
#include <utility>

namespace std {

template <typename... Args>
void print(fmt::format_string<Args...> fmt_str, Args &&...args) {
    fmt::print(fmt_str, std::forward<Args>(args)...);
}

template <typename... Args>
void print(std::ostream &os, fmt::format_string<Args...> fmt_str, Args &&...args) {
    fmt::print(os, fmt_str, std::forward<Args>(args)...);
}

template <typename... Args>
void println(fmt::format_string<Args...> fmt_str, Args &&...args) {
    fmt::println(fmt_str, std::forward<Args>(args)...);
}

template <typename... Args>
void println(std::ostream &os, fmt::format_string<Args...> fmt_str, Args &&...args) {
    fmt::println(os, fmt_str, std::forward<Args>(args)...);
}

} // namespace std

#endif // WIP_HAVE_STD_PRINT
