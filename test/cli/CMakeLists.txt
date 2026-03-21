# test/cli/CMakeLists.txt
#
# Registers each cli integration test as a ctest entry.
#
# TEST_TREE is placed inside the cmake binary directory so ctest manages it.
# Artifacts are NOT removed after the test runs (useful for debugging).
# Before each test its own subdirectory is wiped and re-created by the script.

set(LEGACY_TEST_TREE "${CMAKE_CURRENT_BINARY_DIR}/test-artifacts")
set(GIT_WIP_BIN "$<TARGET_FILE:git-wip>")

foreach(TEST_NAME IN ITEMS test_legacy test_spaces)
    add_test(
        NAME    "cli/${TEST_NAME}"
        COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/${TEST_NAME}.sh"
    )
    set_tests_properties("cli/${TEST_NAME}" PROPERTIES
        ENVIRONMENT "GIT_WIP=${GIT_WIP_BIN};TEST_TREE=${LEGACY_TEST_TREE}"
        TIMEOUT 60
    )
endforeach()
