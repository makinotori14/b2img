cmake_minimum_required(VERSION 3.21)

project(
    b2img
    VERSION 0.1.0
    DESCRIPTION "binary to image explorer"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
    message(FATAL_ERROR "b2img targets Linux only.")
endif()

if(MSVC)
    add_compile_options(/W4 /permissive-)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK4 REQUIRED IMPORTED_TARGET gtk4)

add_library(
    b2img_core
    src/app/render_service.cpp
    src/core/analyzer.cpp
    src/core/decoder.cpp
    src/core/palette.cpp
    src/io/file_buffer.cpp
    src/io/ppm_writer.cpp
)

target_include_directories(b2img_core PUBLIC include)

add_library(
    b2img_cli_support
    src/app/application.cpp
    src/cli/command_line.cpp
)

target_include_directories(b2img_cli_support PUBLIC include)
target_link_libraries(b2img_cli_support PUBLIC b2img_core)

add_executable(
    b2img
    src/gui/main.cpp
    src/gui/main_window.cpp
)

target_link_libraries(
    b2img
    PRIVATE
        b2img_core
        PkgConfig::GTK4
)

add_executable(b2img-cli src/cli_main.cpp)
target_link_libraries(b2img-cli PRIVATE b2img_cli_support)

install(TARGETS b2img b2img-cli RUNTIME DESTINATION bin)
