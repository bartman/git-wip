# test/nvim/CMakeLists.txt
#
# Registers each Neovim integration test as a ctest entry.
#
# TEST_TREE is placed inside the cmake binary directory so ctest manages it.
# Artifacts are NOT removed after the test runs (useful for debugging).
# Before each test its own subdirectory is wiped and re-created by the script.
#
# Tests are skipped if nvim is not found in PATH.

set(NVIM_TEST_TREE "${CMAKE_CURRENT_BINARY_DIR}/test-artifacts")
set(GIT_WIP_BIN "$<TARGET_FILE:git-wip>")

# Find nvim
find_program(NVIM_EXECUTABLE nvim)

if(NVIM_EXECUTABLE)
    message(STATUS "Found nvim: ${NVIM_EXECUTABLE}")

    foreach(TEST_NAME IN ITEMS test_nvim_single test_nvim_buffers test_nvim_windows)
        add_test(
            NAME    "nvim/${TEST_NAME}"
            COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/${TEST_NAME}.sh"
        )
        set_tests_properties("nvim/${TEST_NAME}" PROPERTIES
            ENVIRONMENT "GIT_WIP=${GIT_WIP_BIN};NVIM=${NVIM_EXECUTABLE};TEST_TREE=${NVIM_TEST_TREE}"
            TIMEOUT 120
        )
    endforeach()
else()
    message(STATUS "nvim not found - Neovim tests will be skipped")
endif()
