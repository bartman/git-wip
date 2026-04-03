cmake_minimum_required(VERSION 3.26)
project(git-wip LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable generation of compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(WIP_COVERAGE "Enable code coverage instrumentation" OFF)
option(WIP_STATIC  "Build a fully static binary"           OFF)

include(FetchContent)
include(CheckCXXSourceCompiles)

FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG v1.15.3
)

FetchContent_MakeAvailable(spdlog)

# Code coverage options — applied AFTER FetchContent so that third-party
# dependencies (spdlog, fmt, …) are NOT instrumented.  Mixing gcov versions
# between the host compiler and a pre-built dep causes "version mismatch"
# errors at runtime that pollute test output.
if(WIP_COVERAGE)
    add_compile_options(-fprofile-arcs -ftest-coverage)
    add_link_options(-fprofile-arcs -ftest-coverage)
endif()

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
        GIT_TAG 11.1.4
    )
    FetchContent_MakeAvailable(fmt)
endif()

include(FindPkgConfig)

if(WIP_STATIC)
    message(STATUS "WIP_STATIC=ON — building a mostly-static binary (libgit2 and all its deps statically linked; glibc stays shared)")
    pkg_check_modules(LIBGIT2 REQUIRED libgit2)

    # Collect the full transitive static link flags from pkg-config.
    # We split them into two groups so CMake can handle each token cleanly:
    #   LIBGIT2_STATIC_LIBS  — -lfoo tokens (library names, no -L prefix)
    #   LIBGIT2_STATIC_DIRS  — -L/path tokens (search paths)
    execute_process(
        COMMAND pkg-config --static --libs libgit2
        OUTPUT_VARIABLE _git2_raw
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    execute_process(
        COMMAND pkg-config --static --libs-only-l libgit2
        OUTPUT_VARIABLE _git2_libs_raw
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    execute_process(
        COMMAND pkg-config --static --libs-only-L libgit2
        OUTPUT_VARIABLE _git2_dirs_raw
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    # Also pull in the transitive deps of libssh2 (uses OpenSSL on Debian)
    execute_process(
        COMMAND pkg-config --static --libs-only-l libssh2
        OUTPUT_VARIABLE _ssh2_libs_raw
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    string(REPLACE "-l" "" _git2_libs_stripped "${_git2_libs_raw}")
    string(REPLACE "-L" "" _git2_dirs_stripped "${_git2_dirs_raw}")
    string(REPLACE "-l" "" _ssh2_libs_stripped "${_ssh2_libs_raw}")
    separate_arguments(LIBGIT2_STATIC_LIBS UNIX_COMMAND "${_git2_libs_stripped}")
    separate_arguments(LIBGIT2_STATIC_DIRS UNIX_COMMAND "${_git2_dirs_stripped}")
    separate_arguments(SSH2_STATIC_LIBS    UNIX_COMMAND "${_ssh2_libs_stripped}")
    # Deduplicate (SSH2 libs are a subset)
    list(APPEND LIBGIT2_STATIC_LIBS ${SSH2_STATIC_LIBS})
    list(REMOVE_DUPLICATES LIBGIT2_STATIC_LIBS)
else()
    pkg_check_modules(LIBGIT2 REQUIRED libgit2)
endif()

# Optional: GTest/GMock for unit tests (only available on some distros)
find_package(GTest QUIET)

enable_testing()

if(GTest_FOUND)
    message(STATUS "GTest found — building unit tests")
    add_subdirectory(test/unit)
else()
    message(STATUS "GTest not found — skipping unit tests")
endif()

add_subdirectory(test/cli)
add_subdirectory(test/nvim)

# the executable
add_subdirectory(src)
