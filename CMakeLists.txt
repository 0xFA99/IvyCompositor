cmake_minimum_required(VERSION 3.10)
project(IvyCompositor C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(PkgConfig REQUIRED)

pkg_check_modules(WAYLAND_SERVER REQUIRED wayland-server)
pkg_check_modules(WLROOTS REQUIRED wlroots-0.20)
pkg_check_modules(XKBCOMMON REQUIRED xkbcommon)

find_path(WLR_PROTOCOLS_UNSTABLE_XML_DIR
        NAMES wlr-layer-shell-unstable-v1.xml
        PATHS /usr/share/wlr-protocols/unstable
        /usr/local/share/wlr-protocols/unstable
        REQUIRED
)

set(PROTOCOL_HEADERS
        "${CMAKE_CURRENT_BINARY_DIR}/wlr-layer-shell-unstable-v1-protocol.h"
        "${CMAKE_CURRENT_BINARY_DIR}/wlr-output-power-management-unstable-v1-protocol.h"
)

add_custom_command(
        OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/wlr-layer-shell-unstable-v1-protocol.h"
        COMMAND wayland-scanner server-header
        "${WLR_PROTOCOLS_UNSTABLE_XML_DIR}/wlr-layer-shell-unstable-v1.xml"
        "${CMAKE_CURRENT_BINARY_DIR}/wlr-layer-shell-unstable-v1-protocol.h"
        DEPENDS "${WLR_PROTOCOLS_UNSTABLE_XML_DIR}/wlr-layer-shell-unstable-v1.xml"
        COMMENT "Generating wlr-layer-shell protocol header..."
)

add_custom_command(
        OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/wlr-output-power-management-unstable-v1-protocol.h"
        COMMAND wayland-scanner server-header
        "${WLR_PROTOCOLS_UNSTABLE_XML_DIR}/wlr-output-power-management-unstable-v1.xml"
        "${CMAKE_CURRENT_BINARY_DIR}/wlr-output-power-management-unstable-v1-protocol.h"
        DEPENDS "${WLR_PROTOCOLS_UNSTABLE_XML_DIR}/wlr-output-power-management-unstable-v1.xml"
        COMMENT "Generating wlr-output-power-management protocol header..."
)

add_executable(${PROJECT_NAME}
        src/main.c

        src/core/core.c
        src/core/server.c
        src/core/output.c
        src/core/scene.c

        src/input/input.c
        src/input/seat.c
        src/input/cursor/cursor.c
        src/input/cursor/shape.c
        src/input/keyboard.c
        src/input/idle_inhibitor.c

        src/shell/shell.c
        src/shell/layer/layer_shell.c
        src/shell/xdg/shell.c
        src/shell/xdg/toplevel.c
        src/shell/xdg/decoration.c
        src/shell/xdg/icon.c
        src/shell/xdg/activation.c
        src/shell/xdg/popup.c

        ${PROTOCOL_HEADERS}
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
        -Wshadow
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