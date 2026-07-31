#ifndef IVY_SHELL_TOP_LEVEL_H
#define IVY_SHELL_TOP_LEVEL_H

#include "core/fwd.h"

#include <wayland-server-core.h>

#ifdef __cplusplus
extern "C" {
#endif

struct IvyXdgTopLevel
{
    struct wlr_xdg_toplevel *wlr_xdg_toplevel;
    struct wlr_scene_tree *wlr_scene_tree;

    struct wl_list link;

    struct wl_listener map;
    struct wl_listener unmap;
    struct wl_listener commit;
    struct wl_listener destroy;

    struct wl_listener request_move;
    struct wl_listener request_resize;
    struct wl_listener request_maximize;
    struct wl_listener request_fullscreen;
    struct wl_listener request_activate;
};

struct IvyXdgTopLevelManager {
    struct wl_list toplevels;
    struct wl_listener new_toplevel;
};

void Ivy_XdgTopLevelManager_Init(IvyXdgTopLevelManager *toplevel_manager);

#ifdef __cplusplus
}
#endif

#endif