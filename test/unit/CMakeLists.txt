find_package(GTest REQUIRED)

# Directory where test repos are created (inside the cmake binary tree)
set(UNIT_TEST_TMPDIR "${CMAKE_CURRENT_BINARY_DIR}/repos")

# ---------------------------------------------------------------------------
# Helper macro: add a unit test executable and register it with ctest.
#
# Usage: add_unit_test(NAME <name> SOURCES <file.cpp> [NEEDS_GIT])
#   NEEDS_GIT — also links libgit2 and gets the LIBGIT2 include dirs
# ---------------------------------------------------------------------------
macro(add_unit_test)
    cmake_parse_arguments(UT "" "NAME" "SOURCES;LINKS" ${ARGN})

    add_executable(${UT_NAME} ${UT_SOURCES})

    target_include_directories(${UT_NAME} PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}   # for test_repo_fixture.hpp
        ${LIBGIT2_INCLUDE_DIRS}
    )

    target_link_libraries(${UT_NAME} PRIVATE
        GTest::gtest
        GTest::gtest_main
        ${LIBGIT2_LIBRARIES}
        ${UT_LINKS}
    )

    target_link_directories(${UT_NAME} PRIVATE
        ${LIBGIT2_LIBRARY_DIRS}
    )

    add_test(NAME "unit/${UT_NAME}" COMMAND ${UT_NAME})

    set_tests_properties("unit/${UT_NAME}" PROPERTIES
        ENVIRONMENT "TEST_TMPDIR=${UNIT_TEST_TMPDIR}/${UT_NAME}"
    )
endmacro()

# ---------------------------------------------------------------------------
# test_string_helpers — pure functions, no libgit2 needed at runtime but
# we include the libgit2 headers via LIBGIT2_INCLUDE_DIRS anyway since
# string_helpers.hpp indirectly pulls in git_guards.hpp → git2.h.
# ---------------------------------------------------------------------------
add_unit_test(NAME test_string_helpers SOURCES test_string_helpers.cpp)

# ---------------------------------------------------------------------------
# test_git_helpers — exercises git_helpers.hpp; needs libgit2
# ---------------------------------------------------------------------------
add_unit_test(NAME test_git_helpers SOURCES test_git_helpers.cpp)

# ---------------------------------------------------------------------------
# test_wip_helpers — exercises wip_helpers.hpp; needs libgit2
# ---------------------------------------------------------------------------
add_unit_test(NAME test_wip_helpers SOURCES test_wip_helpers.cpp)
