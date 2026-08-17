#ifndef IVY_SHELL_XWAYLAND_SURFACE_H
#define IVY_SHELL_XWAYLAND_SURFACE_H

#include "core/fwd.h"

#include <wayland-server-core.h>
#include <wlr/xwayland/xwayland.h>

#ifdef __cplusplus
extern "C" {
#endif

struct IvyXwaylandSurface
{
    struct wlr_xwayland_surface *wlr_xwayland_surface;
    struct wlr_scene_tree *wlr_scene_tree;

    IvyServer *server;
    struct wl_list link;

    struct wl_listener associate;
    struct wl_listener dissociate;
    struct wl_listener destroy;

    struct wl_listener map;
    struct wl_listener unmap;
    struct wl_listener commit;

    struct wl_listener request_configure;
    struct wl_listener request_move;
    struct wl_listener request_resize;
    struct wl_listener request_maximize;
    struct wl_listener request_fullscreen;
    struct wl_listener request_activate;
};

IvyXwaylandSurface *Ivy_Xwayland_Create(IvyServer *server, struct wlr_xwayland_surface *wlr_xwayland_surface);

#ifdef __cplusplus
}
#endif

#endif