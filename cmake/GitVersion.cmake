# GitVersion.cmake - Integration for GitVersion.sh
#
# Usage:
#   include(GitVersion)
#   gitversion_generate(PREFIX GIT_WIP_ OUTPUT ${CMAKE_BINARY_DIR}/git_wip_version.h)

function(gitversion_generate)
    set(options)
    set(oneValueArgs PREFIX OUTPUT)
    set(multiValueArgs)

    cmake_parse_arguments(GV "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(NOT GV_PREFIX)
        message(FATAL_ERROR "gitversion_generate: PREFIX is required")
    endif()
    if(NOT GV_OUTPUT)
        message(FATAL_ERROR "gitversion_generate: OUTPUT is required")
    endif()

    # Run GitVersion.sh to generate the version header
    execute_process(
        COMMAND ${CMAKE_SOURCE_DIR}/cmake/GitVersion.sh ${GV_PREFIX} ${GV_OUTPUT}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        RESULT_VARIABLE GV_RESULT
    )

    if(NOT GV_RESULT EQUAL 0)
        message(WARNING "gitversion_generate: Failed to run GitVersion.sh")
    endif()

    # Add a custom command to regenerate the version header
    add_custom_command(
        OUTPUT ${GV_OUTPUT}
        COMMAND ${CMAKE_SOURCE_DIR}/cmake/GitVersion.sh ${GV_PREFIX} ${GV_OUTPUT}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        DEPENDS ${CMAKE_SOURCE_DIR}/cmake/GitVersion.sh
        COMMENT "Generating version header: ${GV_OUTPUT}"
        VERBATIM
    )

    # Add a custom target that depends on the version header
    add_custom_target(gitversion DEPENDS ${GV_OUTPUT})
endfunction()
