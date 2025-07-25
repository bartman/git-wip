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

# Your executable
add_executable(git-wip
    src/main.cpp
    src/cmd_config.cpp
    src/cmd_delete.cpp
    src/cmd_log.cpp
    src/cmd_save.cpp
    src/cmd_status.cpp
)

target_include_directories(git-wip PRIVATE
    ${clipp_SOURCE_DIR}/include
    ${LIBGIT2_INCLUDE_DIRS}
)

target_link_libraries(git-wip PRIVATE
    spdlog::spdlog
    ${LIBGIT2_LIBRARIES}
)

target_link_directories(git-wip PRIVATE
    ${LIBGIT2_LIBRARY_DIRS}
)

