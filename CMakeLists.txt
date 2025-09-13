cmake_minimum_required(VERSION 3.31)
project(mole)

set(CMAKE_CXX_STANDARD 26)

if (MSVC)
    set(CMAKE_CXX_FLAGS_DEBUG_INIT "/ZI")
endif ()

option(CMAKE_MOLE_TEST "Build tests for mole" OFF)

if (DEFINED MOLE_BUILD_TESTS)
    set(MOLE_TEST_ENABLED ${MOLE_BUILD_TESTS})
else ()
    if (CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(MOLE_TEST_ENABLED ON)
    else ()
        set(MOLE_TEST_ENABLED OFF)
    endif ()
endif ()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

find_package(Threads REQUIRED)

set(SPDLOG_USE_STD_FORMAT ON)
set(SPDLOG_NO_EXCEPTIONS ON)

include(FetchContent)
FetchContent_Declare(
        spdlog
        GIT_REPOSITORY https://github.com/gabime/spdlog
        GIT_TAG v1.15.3
        SYSTEM)
FetchContent_MakeAvailable(spdlog)

FetchContent_Declare(
        minhook
        GIT_REPOSITORY https://github.com/TsudaKageyu/minhook
        GIT_TAG v1.3.4
        SYSTEM)
FetchContent_MakeAvailable(minhook)

add_library(mole MODULE main.cpp
        logging/logger.cpp
        logging/logger.h
        threads/threads.cpp
        threads/threads.h
        g_flags/flags.h
        g_flags/flags.cpp
        hooks/memory.cpp
        hooks/memory.h
        unload/unloader.cpp
        unload/unloader.h
        memory/resolve_symbol.cpp
        memory/resolve_symbol.h 
        host/host.h 
        host/host.cpp 
        commands/commands.h 
        commands/commands.cpp 
        aggregate/common.h)
target_link_libraries(mole spdlog::spdlog minhook Dbghelp.lib)
include_directories(aggregate)

if (MOLE_TEST_ENABLED)
    file(GLOB TEST_SOURCES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/tests/*.cpp")

    foreach (TEST_SRC ${TEST_SOURCES})
        get_filename_component(TEST_NAME ${TEST_SRC} NAME_WE)

        add_executable(${TEST_NAME} ${TEST_SRC})

        if (TEST_NAME MATCHES ".*_gui$")
            set_target_properties(${TEST_NAME} PROPERTIES
                    WIN32_EXECUTABLE TRUE
            )
        endif ()

        add_dependencies(${TEST_NAME} mole)

        target_link_libraries(${TEST_NAME} PRIVATE Threads::Threads)

        add_custom_command(TARGET ${TEST_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_FILE:mole>
                $<TARGET_FILE_DIR:${TEST_NAME}>
        )
    endforeach ()
endif ()
