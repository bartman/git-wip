add_executable(git-wip
    main.cpp
    cmd_delete.cpp
    cmd_list.cpp
    cmd_log.cpp
    cmd_save.cpp
    cmd_status.cpp
)

install(TARGETS git-wip
    RUNTIME DESTINATION bin
)

target_include_directories(git-wip PRIVATE
    ${LIBGIT2_INCLUDE_DIRS}
)

target_link_libraries(git-wip PRIVATE
    spdlog::spdlog
    ${LIBGIT2_LIBRARIES}
)

target_link_directories(git-wip PRIVATE
    ${LIBGIT2_LIBRARY_DIRS}
)

if(WIP_HAVE_STD_PRINT)
    target_compile_definitions(git-wip PRIVATE WIP_HAVE_STD_PRINT)
else()
    target_link_libraries(git-wip PRIVATE fmt::fmt)
endif()
