cmake_minimum_required(VERSION 3.31)
project(mole_ui)

set(CMAKE_CXX_STANDARD 26)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(${CMAKE_SOURCE_DIR}/tools ${CMAKE_BINARY_DIR}/tools)
include(${CMAKE_SOURCE_DIR}/cmake/compile_assets.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/windows_resources.cmake)

set(GLFW_BUILD_EXAMPLES OFF)
set(GLFW_BUILD_TESTS OFF)
set(GLFW_BUILD_DOCS OFF)

find_package(OpenGL REQUIRED)

include(FetchContent)
FetchContent_Declare(
        imgui_src
        GIT_REPOSITORY https://github.com/ocornut/imgui
        GIT_TAG v1.92.2b-docking
        SYSTEM)
FetchContent_MakeAvailable(imgui_src)

FetchContent_Declare(
        glfw
        GIT_REPOSITORY https://github.com/glfw/glfw
        GIT_TAG 3.4
        SYSTEM
)
FetchContent_MakeAvailable(glfw)

add_library(imgui
        ${imgui_src_SOURCE_DIR}/imgui.cpp
        ${imgui_src_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_src_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_src_SOURCE_DIR}/imgui_widgets.cpp
        ${imgui_src_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
        ${imgui_src_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)
target_link_libraries(imgui PUBLIC glfw OpenGL::GL)
target_include_directories(imgui PUBLIC ${imgui_src_SOURCE_DIR} ${imgui_src_SOURCE_DIR}/backends)

#mole_compile_binary_to_header(
#        INPUT_FILE ${CMAKE_SOURCE_DIR}/mole_ui/assets/Iosevka-Regular.ttc
#        OUTPUT_BASE ${CMAKE_BINARY_DIR}/include/generated/fonts/Iosevka-Regular
#        ARRAY_NAME iosevka_regular
#)

add_executable(mole_ui
        main.cpp
        main.h
        aggregate/common.h
        ipc/client.cpp
        ipc/client.h
        ui/root.cpp
        ui/root.h
        commands/commands.cpp
        commands/commands.h
        resources.h
        #        ${iosevka_regular_C}
        #        ${iosevka_regular_H}
)
#add_dependencies(mole_ui bin2c)

mole_add_resource(
        TARGET mole_ui
        TYPE RCDATA
        ID IDR_IOSEVKA_REGULAR
        FILE ${CMAKE_SOURCE_DIR}/mole_ui/assets/Iosevka-Regular.ttc
)

mole_generate_resources_rc(
        TARGET mole_ui
        ADD_TO_TARGET
)

target_link_libraries(mole_ui PUBLIC imgui nng)
target_link_libraries(mole_ui PRIVATE gdi32 opengl32)
target_include_directories(mole_ui PUBLIC aggregate)

target_compile_definitions(mole_ui PRIVATE $<$<CONFIG:Release>:MOLE_RELEASE>)

set_target_properties(mole_ui PROPERTIES
        WIN32_EXECUTABLE "$<$<CONFIG:Release>:TRUE>$<$<CONFIG:Debug>:FALSE>"
)