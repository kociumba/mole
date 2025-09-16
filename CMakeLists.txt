cmake_minimum_required(VERSION 3.31)
project(mole)

set(CMAKE_CXX_STANDARD 26)

if (MSVC)
    set(CMAKE_CXX_FLAGS_DEBUG_INIT "/ZI")
endif ()

option(CMAKE_MOLE_TEST "Build tests for mole" OFF)
option(CMAKE_BUILD_MOLE_UI "Build the external interface for mole" ON)

if (CMAKE_MOLE_TEST)
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

add_library(mole MODULE mole/main.cpp
        mole/logging/logger.cpp
        mole/logging/logger.h
        mole/threads/threads.cpp
        mole/threads/threads.h
        mole/g_flags/flags.h
        mole/g_flags/flags.cpp
        mole/hooks/memory.cpp
        mole/hooks/memory.h
        mole/unload/unloader.cpp
        mole/unload/unloader.h
        mole/memory/resolve_symbol.cpp
        mole/memory/resolve_symbol.h
        mole/host/host.h
        mole/host/host.cpp
        mole/commands/commands.h
        mole/commands/commands.cpp
        mole/aggregate/common.h
        mole/ipc/server.h
        mole/ipc/server.cpp)
target_link_libraries(mole spdlog::spdlog minhook Dbghelp.lib)
target_include_directories(mole PUBLIC mole/aggregate)
if (MSVC)
    target_compile_options(mole PUBLIC /EHs-)
endif ()

if (CMAKE_BUILD_MOLE_UI)
    message(STATUS "configuring the build for mole mole_ui")
    add_subdirectory(mole_ui)
endif ()

if (MOLE_TEST_ENABLED)
    file(GLOB TEST_SOURCES CONFIGURE_DEPENDS " ${CMAKE_SOURCE_DIR}/mole/tests/*.cpp")

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
