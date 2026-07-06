#ifndef IVY_TOP_LEVEL_H
#define IVY_TOP_LEVEL_H

#include "fwd.h"

#include <wayland-server-core.h>
#include <wayland-util.h>

#ifdef __cplusplus
extern "C" {
#endif

struct IvyTopLevel {
    IvyServer *server;
    struct wl_list link;
    struct wlr_xdg_toplevel *xdg_toplevel;
    struct wlr_scene_tree *scene_tree;

    struct wl_listener map;
    struct wl_listener unmap;
    struct wl_listener commit;

    struct wl_listener destroy;
};

void Ivy_Server_HandleNewXdgTopLevel(struct wl_listener *listener, void *data);

#ifdef __cplusplus
}
#endif

#endif