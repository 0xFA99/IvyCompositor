#ifndef IVY_TOP_LEVEL_H
#define IVY_TOP_LEVEL_H

#include "fwd.h"

#include <wayland-server-core.h>
#include <wayland-util.h>
#include <wlr/util/box.h>

#ifdef __cplusplus
extern "C" {
#endif

enum IvyTopLevelType {
    IVY_TOPLEVEL_XDG,
    IVY_TOPLEVEL_XWAYLAND,
};

struct IvyTopLevel {
    IvyServer *server;
    struct wl_list link;
    enum IvyTopLevelType type;
    union {
        struct wlr_xdg_toplevel *xdg_toplevel;
        struct wlr_xwayland_surface *xwayland_surface;
    };
    struct wlr_scene_tree *scene_tree;
    struct wlr_scene_tree *content_tree;

    struct wl_listener map;
    struct wl_listener unmap;
    struct wl_listener commit;

    struct wl_listener destroy;

    struct wl_listener request_move;
    struct wl_listener request_resize;

    struct wl_listener request_maximize;
    struct wl_listener request_fullscreen;
    struct wl_listener request_activate;
    struct wl_listener request_configure;

    struct wl_listener associate;
    struct wl_listener dissociate;

    struct wlr_xdg_toplevel_decoration_v1 *xdg_decoration;
    struct wl_listener decoration_request_mode;
    struct wl_listener decoration_destroy;

    struct wlr_scene_rect *border_top;
    struct wlr_scene_rect *border_bottom;
    struct wlr_scene_rect *border_left;
    struct wlr_scene_rect *border_right;

    bool is_maximized;
    bool is_fullscreen;
    struct wlr_box saved_geometry;
    int workspace;
};

void Ivy_Server_HandleNewXdgTopLevel(struct wl_listener *listener, void *data);
void Ivy_Server_HandleNewXWaylandSurface(struct wl_listener *listener, void *data);
void Ivy_Server_HandleNewXdgDecoration(struct wl_listener *listener, void *data);

void Ivy_TopLevel_Focus(IvyTopLevel *topLevel);
void Ivy_TopLevel_SetMaximize(IvyTopLevel *topLevel, bool maximize);
void Ivy_TopLevel_SetFullscreen(IvyTopLevel *topLevel, bool fullscreen);
void Ivy_TopLevel_UpdateBorders(IvyTopLevel *topLevel);

#ifdef __cplusplus
}
#endif

#endif