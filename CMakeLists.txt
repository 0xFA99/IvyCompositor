cmake_minimum_required(VERSION 3.10)
project(IvyCompositor C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(PkgConfig REQUIRED)

pkg_check_modules(WAYLAND_SERVER REQUIRED wayland-server)
pkg_check_modules(WLROOTS REQUIRED wlroots-0.20)
pkg_check_modules(XKBCOMMON REQUIRED xkbcommon)

execute_process(
        COMMAND wayland-scanner server-header
        /usr/share/wlr-protocols/unstable/wlr-layer-shell-unstable-v1.xml
        ${CMAKE_CURRENT_BINARY_DIR}/wlr-layer-shell-unstable-v1-protocol.h
)

add_executable(${PROJECT_NAME}
        src/main.c
        src/server.c
        src/output.c
        src/top_level.c
        src/input.c
        src/keyboard.c
        src/cursor.c
        src/popup.c
        src/layer_surface.c
)

target_include_directories(${PROJECT_NAME} PRIVATE
        include
        ${CMAKE_CURRENT_BINARY_DIR}
        ${WAYLAND_SERVER_INCLUDE_DIRS}
        ${WLROOTS_INCLUDE_DIRS}
        ${XKBCOMMON_INCLUDE_DIRS}
)

target_compile_definitions(${PROJECT_NAME} PRIVATE WLR_USE_UNSTABLE)

target_compile_options(${PROJECT_NAME} PRIVATE
        -Wall -Wextra -Wno-unused-parameter
        ${WAYLAND_SERVER_CFLAGS_OTHER}
        ${WLROOTS_CFLAGS_OTHER}
        ${XKBCOMMON_CFLAGS_OTHER}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
        ${WAYLAND_SERVER_LIBRARIES}
        ${WLROOTS_LIBRARIES}
        ${XKBCOMMON_LIBRARIES}
)

target_link_directories(${PROJECT_NAME} PRIVATE
        ${WAYLAND_SERVER_LIBRARY_DIRS}
        ${WLROOTS_LIBRARY_DIRS}
        ${XKBCOMMON_LIBRARY_DIRS}
)