# Generate version header
set(VERSION_HEADER "")
set(VERSION_HEADER_DIR "${CMAKE_BINARY_DIR}")

if(DEFINED USE_GIT_WIP_VERSION_H AND EXISTS "${USE_GIT_WIP_VERSION_H}")
    set(VERSION_HEADER "${USE_GIT_WIP_VERSION_H}")
    get_filename_component(VERSION_HEADER_DIR "${VERSION_HEADER}" DIRECTORY)
    message(STATUS "Using pre-generated version header: ${VERSION_HEADER}")
    add_custom_target(gitversion DEPENDS "${VERSION_HEADER}")
else()
    if(DEFINED USE_GIT_WIP_VERSION_H)
        message(WARNING "USE_GIT_WIP_VERSION_H was set but does not exist: ${USE_GIT_WIP_VERSION_H}")
    endif()

    # Normal development path (local make, dev shell, etc.)
    message(STATUS "No pre-generated header found → generating with GitVersion.sh")
    include(${CMAKE_SOURCE_DIR}/cmake/GitVersion.cmake)
    gitversion_generate(PREFIX GIT_WIP_ OUTPUT ${CMAKE_BINARY_DIR}/git_wip_version.h)
    set(VERSION_HEADER "${CMAKE_BINARY_DIR}/git_wip_version.h")
    set(VERSION_HEADER_DIR "${CMAKE_BINARY_DIR}")
endif()

add_executable(git-wip
    color.cpp
    config.cpp
    main.cpp
    cmd_delete.cpp
    cmd_list.cpp
    cmd_log.cpp
    cmd_save.cpp
    cmd_status.cpp
)

# Ensure the executable is rebuilt when the version header changes
add_dependencies(git-wip gitversion)

install(TARGETS git-wip
    RUNTIME DESTINATION bin
)

target_include_directories(git-wip PRIVATE
    ${VERSION_HEADER_DIR}
    ${LIBGIT2_INCLUDE_DIRS}
)

if(WIP_STATIC)
    # Link libgit2 and all its transitive deps as static archives.
    # Strategy:
    #   1. -Wl,-Bstatic wraps the static block.
    #   2. --start-group / --end-group around all static libs resolves any
    #      circular or out-of-order symbol dependencies (e.g. libcrypto → libz).
    #   3. -Wl,-Bdynamic switches back to dynamic for the remainder
    #      (glibc, gssapi_krb5 and its kerberos chain have no static .a here).
    #   4. GSSAPI / krb5 are added explicitly as dynamic-only after the switch.
    #
    # spdlog is always a static archive (built by FetchContent) and does NOT
    # need to be inside -Bstatic because CMake links it by full path anyway.
    target_link_directories(git-wip PRIVATE
        ${LIBGIT2_STATIC_DIRS}
    )
    set(_static_lib_args
        "-Wl,-Bstatic"
        "-Wl,--start-group"
    )
    foreach(_lib IN LISTS LIBGIT2_STATIC_LIBS)
        list(APPEND _static_lib_args "-l${_lib}")
    endforeach()
    list(APPEND _static_lib_args
        "-Wl,--end-group"
        "-Wl,-Bdynamic"
        # GSSAPI / Kerberos — only available as shared libs on most distros
        "-lgssapi_krb5"
        "-lkrb5"
        "-lk5crypto"
        "-lkrb5support"
        "-lcom_err"
    )
    target_link_libraries(git-wip PRIVATE
        spdlog::spdlog
        ${_static_lib_args}
    )
else()
    target_link_libraries(git-wip PRIVATE
        spdlog::spdlog
        ${LIBGIT2_LIBRARIES}
    )
    target_link_directories(git-wip PRIVATE
        ${LIBGIT2_LIBRARY_DIRS}
    )
endif()

if(WIP_HAVE_STD_PRINT)
    target_compile_definitions(git-wip PRIVATE WIP_HAVE_STD_PRINT)
else()
    target_link_libraries(git-wip PRIVATE fmt::fmt)
endif()
