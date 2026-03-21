cmake_minimum_required(VERSION 3.10)

find_package(Python3 REQUIRED)

set(CAP_RUN "${CMAKE_SOURCE_DIR}/test/cap-run.sh")

set(TEST_SCRIPT "${CMAKE_SOURCE_DIR}/test/runner.py")
set(TEST_JSON "${CMAKE_CURRENT_SOURCE_DIR}/runner-test.json")

set(DRY_EXPECTED "${CMAKE_CURRENT_SOURCE_DIR}/runner-test.dry-run.output")
set(BLIND_EXPECTED "${CMAKE_CURRENT_SOURCE_DIR}/runner-test.blind-run.output")

set(DRY_OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/runner.dry-run.out")
set(BLIND_OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/runner.blind-run.out")

# Dry-run test
add_test(NAME runner_dry_run
  COMMAND ${CAP_RUN} ${DRY_OUTPUT} ${Python3_EXECUTABLE} ${TEST_SCRIPT}
    --dry-run
    --input ${TEST_JSON}
    TOOL=echo ONE=1 TWO=22 THREE=333 FOUR=4444
)
set_tests_properties(runner_dry_run PROPERTIES
  PASS_REGULAR_EXPRESSION "Executing:"
)
add_test(NAME runner_dry_run_check
  COMMAND ${CMAKE_COMMAND} -E compare_files ${DRY_EXPECTED} ${DRY_OUTPUT}
)

# Blind-run test
add_test(NAME runner_blind_run
  COMMAND ${CAP_RUN} ${BLIND_OUTPUT} ${Python3_EXECUTABLE} ${TEST_SCRIPT}
    --blind-run
    --input ${TEST_JSON}
    TOOL=echo ONE=1 TWO=22 THREE=333 FOUR=4444
)
set_tests_properties(runner_blind_run PROPERTIES
  PASS_REGULAR_EXPRESSION "Executing:"
)
add_test(NAME runner_blind_run_check
  COMMAND ${CMAKE_COMMAND} -E compare_files ${BLIND_EXPECTED} ${BLIND_OUTPUT}
)
