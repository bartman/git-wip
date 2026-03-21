find_package(GTest REQUIRED)

# Unit test for string_helpers.hpp (pure functions, no libgit2 dependency)
add_executable(test_string_helpers
    test_string_helpers.cpp
)
target_include_directories(test_string_helpers PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)
target_link_libraries(test_string_helpers PRIVATE
    GTest::gtest
    GTest::gtest_main
)
add_test(NAME unit/test_string_helpers COMMAND test_string_helpers)
