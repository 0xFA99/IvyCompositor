cmake_minimum_required(VERSION 3.10)
project(IvyCompositor C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(PkgConfig REQUIRED)

pkg_check_modules(WAYLAND_SERVER REQUIRED wayland-server)
pkg_check_modules(WLROOTS REQUIRED wlroots-0.20)
pkg_check_modules(XKBCOMMON REQUIRED xkbcommon)

add_executable(ivy_compositor
        src/main.c
        src/server.c
        src/output.c
)

target_include_directories(ivy_compositor PRIVATE
        include
        ${WAYLAND_SERVER_INCLUDE_DIRS}
        ${WLROOTS_INCLUDE_DIRS}
        ${XKBCOMMON_INCLUDE_DIRS}
)

target_compile_definitions(ivy_compositor PRIVATE WLR_USE_UNSTABLE)

target_compile_options(ivy_compositor PRIVATE
        -Wall -Wextra -Wno-unused-parameter
        ${WAYLAND_SERVER_CFLAGS_OTHER}
        ${WLROOTS_CFLAGS_OTHER}
        ${XKBCOMMON_CFLAGS_OTHER}
)

target_link_libraries(ivy_compositor PRIVATE
        ${WAYLAND_SERVER_LIBRARIES}
        ${WLROOTS_LIBRARIES}
        ${XKBCOMMON_LIBRARIES}
)

target_link_directories(ivy_compositor PRIVATE
        ${WAYLAND_SERVER_LIBRARY_DIRS}
        ${WLROOTS_LIBRARY_DIRS}
        ${XKBCOMMON_LIBRARY_DIRS}
)