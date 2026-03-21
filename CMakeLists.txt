cmake_minimum_required(VERSION 3.26)
project(git-wip LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable generation of compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)

FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG v1.14.1
)
FetchContent_Declare(
    clipp
    GIT_REPOSITORY https://github.com/muellan/clipp.git
    GIT_TAG v1.2.3
)

# Fetch and populate both
FetchContent_MakeAvailable(spdlog clipp)

include(FindPkgConfig)

pkg_check_modules(LIBGIT2 REQUIRED libgit2)

enable_testing()
add_subdirectory(test/unit)
add_subdirectory(test/runner-test)
add_subdirectory(test/cli)

# the executable
add_subdirectory(src)
