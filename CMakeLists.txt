cmake_minimum_required(VERSION 3.26)
project(git-wip LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable generation of compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)
include(CheckCXXSourceCompiles)

FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG v1.15.3
)
FetchContent_Declare(
    clipp
    GIT_REPOSITORY https://github.com/muellan/clipp.git
    GIT_TAG v1.2.3
)

FetchContent_MakeAvailable(spdlog clipp)

# Detect whether the compiler's standard library ships <print> (C++23 P2093).
# GCC < 14 and some older clangs lack it even with -std=c++23.
check_cxx_source_compiles("
    #include <print>
    int main() { std::println(\"ok\"); }
" WIP_HAVE_STD_PRINT)

if(WIP_HAVE_STD_PRINT)
    message(STATUS "std::print available — using <print>")
else()
    message(STATUS "std::print not available — fetching {fmt} as fallback")
    FetchContent_Declare(
        fmt
        GIT_REPOSITORY https://github.com/fmtlib/fmt.git
        GIT_TAG 12.1.0
    )
    FetchContent_MakeAvailable(fmt)
endif()

include(FindPkgConfig)

pkg_check_modules(LIBGIT2 REQUIRED libgit2)

enable_testing()
add_subdirectory(test/unit)
add_subdirectory(test/runner-test)
add_subdirectory(test/cli)

# the executable
add_subdirectory(src)
