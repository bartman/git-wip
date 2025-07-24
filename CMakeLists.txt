cmake_minimum_required(VERSION 3.26)
project(git-wip LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

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

# Your executable
add_executable(git-wip src/main.cpp)

target_link_libraries(git-wip PRIVATE spdlog::spdlog)
target_include_directories(git-wip PRIVATE ${clipp_SOURCE_DIR}/include)
