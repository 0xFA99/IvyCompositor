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
    struct wl_listener              new_xdg_popup;
    struct wl_list                  topLevels;

    struct wlr_seat                 *seat;
    struct wl_listener              new_input;
    struct wl_list                  keyboards;

    IvyCursor                       *cursor;
    struct wl_listener              request_cursor;
    struct wl_listener              request_set_selection;
    struct wl_listener              pointer_focus_change;

    int                             current_workspace;

    struct wlr_layer_shell_v1       *layer_shell;
    struct wl_listener              new_layer_surface;

    struct wlr_xwayland             *xwayland;
    struct wl_listener              new_xwayland_surface;

    struct wlr_xdg_decoration_manager_v1 *xdg_decoration_manager;
    struct wl_listener              new_xdg_decoration;

    struct wlr_idle_notifier_v1         *idle_notifier;
    struct wlr_idle_inhibit_manager_v1  *idle_inhibit_manager;
    struct wl_listener                  new_idle_inhabitor;

    struct wlr_output_power_manager_v1  *output_power_manager;
    struct wl_listener                  output_power_set_mode;

    struct wlr_xdg_activation_v1        *xdg_activation;
    struct wl_listener                  request_activate;

    struct wlr_scene_tree           *scene_background;
    struct wlr_scene_tree           *scene_bottom;
    struct wlr_scene_tree           *scene_toplevel;
    struct wlr_scene_tree           *scene_top;
    struct wlr_scene_tree           *scene_overlay;
};

void Ivy_Server_Init(IvyServer *server);
void Ivy_Server_Run(const IvyServer *restrict server, const char *restrict cmd);
void Ivy_Server_Destroy(IvyServer *server);
void Ivy_Server_SwitchWorkspace(IvyServer *server, int workspace);
void Ivy_TopLevel_MoveToWorkspace(IvyTopLevel *topLevel, int workspace);

#ifdef __cplusplus
}
#endif

#endif