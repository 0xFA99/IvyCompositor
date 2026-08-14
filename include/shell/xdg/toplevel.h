#ifndef IVY_SHELL_TOP_LEVEL_H
#define IVY_SHELL_TOP_LEVEL_H

#include "core/fwd.h"

#include <wayland-server-core.h>

#ifdef __cplusplus
extern "C" {
#endif

struct wlr_surface;

struct IvyXdgToplevel
{
    struct wlr_xdg_toplevel *wlr_xdg_toplevel;
    struct wlr_scene_tree *wlr_scene_tree;

    IvyServer *server;
    struct wl_list link;

    struct wl_listener map;
    struct wl_listener unmap;
    struct wl_listener commit;
    struct wl_listener destroy;

    struct wl_listener request_move;
    struct wl_listener request_resize;
    struct wl_listener request_maximize;
    struct wl_listener request_fullscreen;
};

struct IvyXdgToplevelManager {
    struct wl_list toplevels;
    struct wl_listener new_toplevel;
};

void Ivy_XdgToplevelManager_Init(IvyXdgToplevelManager *toplevel_manager);
void Ivy_XdgToplevelManager_Destroy(IvyXdgToplevelManager *toplevel_manager);
IvyXdgToplevel *Ivy_XdgToplevel_SurfaceAt(IvyServer *server, double lx, double ly, struct wlr_surface **surface, double *sx, double *sy);
void Ivy_XdgToplevel_Focus(IvyXdgToplevel *toplevel);

#ifdef __cplusplus
}
#endif

#endif