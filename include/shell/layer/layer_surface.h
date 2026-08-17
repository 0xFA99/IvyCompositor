#ifndef IVY_SHELL_LAYER_SURFACE_H
#define IVY_SHELL_LAYER_SURFACE_H

#include "core/fwd.h"

#include <wayland-server-core.h>
#include <wayland-util.h>
#include <wlr/types/wlr_scene.h>

#ifdef __cplusplus
extern "C" {
#endif

struct IvyLayerSurface {
    struct wlr_layer_surface_v1         *wlr_layer_surface;
    struct wlr_scene_layer_surface_v1   *wlr_scene_layer_surface;

    IvyServer          *server;
    struct wl_list      link;

    struct wlr_box      usable_area;

    struct wl_listener  map;
    struct wl_listener  unmap;
    struct wl_listener  commit;
    struct wl_listener  destroy;
};

struct IvyLayerSurfaceManager {
    struct wl_list surfaces;
    struct wl_listener new_surface;
};

void Ivy_LayerSurfaceManager_Init(IvyLayerSurfaceManager *manager);

#ifdef __cplusplus
}
#endif

#endif