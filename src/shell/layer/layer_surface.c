#include "core/server.h"
#include "shell/layer/layer_shell.h"
// #include "shell/layer_surface.h"

#include <wayland-server-core.h>
#include <wayland-util.h>
#include <wlr/types/wlr_layer_shell_v1.h>

#include <stdlib.h>

static void IvyLayerSurface_HandleMap(struct wl_listener *listener, void *data);
static void IvyLayerSurface_HandleUnmap(struct wl_listener *listener, void *data);
static void IvyLayerSurface_HandleCommit(struct wl_listener *listener, void *data);
static void IvyLayerSurface_HandleDestroy(struct wl_listener *listener, void *data);

void Ivy_LayerSurfaceManager_HandleNewSurface(struct wl_listener *listener, void *data)
{
    IvyLayerSurfaceManager *surface_manager = wl_container_of(listener, surface_manager, new_surface);
    struct wlr_layer_surface_v1 *wlr_layer_surface = data;

    IvyLayerShell *layer_shell = wl_container_of(surface_manager, layer_shell, surface_manager);
    IvyServer *server = wl_container_of(layer_shell, server, shell.layer_shell);

    IvyLayerSurface *layer_surface = calloc(1, sizeof(IvyLayerSurface));
    IVY_CHECK(layer_surface != NULL, "[WARNING] Failed to allocate IvyLayerSurface!");

    layer_surface->wlr_layer_surface = wlr_layer_surface;

    struct wlr_scene_tree *parent_tree;
    switch (wlr_layer_surface->pending.layer)
    {
        case ZWLR_LAYER_SHELL_V1_LAYER_BACKGROUND: parent_tree = server->scene.background; break;
        case ZWLR_LAYER_SHELL_V1_LAYER_BOTTOM:     parent_tree = server->scene.bottom; break;
        case ZWLR_LAYER_SHELL_V1_LAYER_TOP:        parent_tree = server->scene.top; break;
        case ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY:    parent_tree = server->scene.overlay; break;
        default:                                   parent_tree = server->scene.top; break;
    }

    layer_surface->wlr_scene_layer_surface = wlr_scene_layer_surface_v1_create(parent_tree, wlr_layer_surface);

    layer_surface->map.notify = IvyLayerSurface_HandleMap;
    wl_signal_add(&wlr_layer_surface->surface->events.map, &layer_surface->map);

    layer_surface->unmap.notify = IvyLayerSurface_HandleUnmap;
    wl_signal_add(&wlr_layer_surface->surface->events.unmap, &layer_surface->unmap);

    layer_surface->commit.notify = IvyLayerSurface_HandleCommit;
    wl_signal_add(&wlr_layer_surface->surface->events.commit, &layer_surface->commit);

    layer_surface->destroy.notify = IvyLayerSurface_HandleDestroy;
    wl_signal_add(&wlr_layer_surface->events.destroy, &layer_surface->destroy);

    wl_list_insert(&surface_manager->surfaces, &layer_surface->link);
}

static void IvyLayerSurface_HandleMap(struct wl_listener *listener, void *data)
{
    IvyLayerSurface *layer_surface = wl_container_of(listener, layer_surface, map);
    // TODO: implement layer surface map
}

static void IvyLayerSurface_HandleUnmap(struct wl_listener *listener, void *data)
{
    IvyLayerSurface *layer_surface = wl_container_of(listener, layer_surface, map);
    // TODO: implement layer surface unmap
}

static void IvyLayerSurface_HandleCommit(struct wl_listener *listener, void *data)
{
    IvyLayerSurface *layer_surface = wl_container_of(listener, layer_surface, commit);

    if (layer_surface->wlr_layer_surface->initial_commit)
    {
        wlr_layer_surface_v1_configure(
            layer_surface->wlr_layer_surface,
            layer_surface->wlr_layer_surface->current.desired_width,
            layer_surface->wlr_layer_surface->current.desired_height);
    }
}

static void IvyLayerSurface_HandleDestroy(struct wl_listener *listener, void *data)
{
    IvyLayerSurface *layer_surface = wl_container_of(listener, layer_surface, destroy);

    wl_list_remove(&layer_surface->map.link);
    wl_list_remove(&layer_surface->unmap.link);
    wl_list_remove(&layer_surface->commit.link);
    wl_list_remove(&layer_surface->destroy.link);

    wl_list_remove(&layer_surface->link);

    free(layer_surface);
}
