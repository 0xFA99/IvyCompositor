#ifndef IVY_SERVER_H
#define IVY_SERVER_H

#include "fwd.h"

#include <wayland-util.h>
#include <wayland-server-core.h>

#ifdef __cplusplus
extern "C" {
#endif

struct IvyServer {
    struct wl_display               *wl_display;
    struct wlr_backend              *backend;
    struct wlr_renderer             *renderer;
    struct wlr_allocator            *allocator;

    struct wlr_output_layout        *output_layout;
    struct wlr_scene                *scene;
    struct wlr_scene_output_layout  *scene_layout;
    struct wlr_scene_rect           *background;

    struct wl_list                  outputs;
    struct wl_listener              new_output;

    struct wlr_xdg_shell            *xdg_shell;
    struct wl_listener              new_xdg_topLevel;
    struct wl_list                  topLevels;

    struct wlr_seat                 *seat;
    struct wl_listener              new_input;
    struct wl_list                  keyboards;

    IvyCursor                       *cursor;
    struct wl_listener              request_cursor;
    struct wl_listener              request_set_selection;
    struct wl_listener              pointer_focus_change;
};

void Ivy_Server_Init(IvyServer *server);
void Ivy_Server_Run(const IvyServer *restrict server, const char *restrict cmd);
void Ivy_Server_Destroy(const IvyServer *server);

#ifdef __cplusplus
}
#endif

#endif