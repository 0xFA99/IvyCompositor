#ifndef IVY_LAYER_SURFACE_H
#define IVY_LAYER_SURFACE_H

#include "fwd.h"

#include <wayland-server-core.h>

#ifdef __cplusplus
extern "C" {
#endif

struct IvyLayerSurface {
    IvyServer                           *server;
    struct wlr_layer_surface_v1         *wlr_layer_surface;
    struct wlr_scene_layer_surface_v1   *scene_layer_surface;
    struct wl_list                      link;

    struct wl_listener                  map;
    struct wl_listener                  unmap;
    struct wl_listener                  commit;
    struct wl_listener                  destroy;
};

void Ivy_Server_HandleNewLayerSurface(struct wl_listener *listener, void *data);

#ifdef __cplusplus
}
#endif

#endif
