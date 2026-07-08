#include "types.h"
#include "server.h"
#include "output.h"
#include "layer_surface.h"

#include <wlr/types/wlr_scene.h>
#include <wlr/types/wlr_output.h>
#include <wlr/types/wlr_layer_shell_v1.h>

#include <stdlib.h>

static void IvyLayerSurface_HandleMap(struct wl_listener *listener, void *data)
{
    IvyLayerSurface *layer_surface = wl_container_of(listener, layer_surface, map);
    (void)data;

    wlr_scene_node_set_enabled(&layer_surface->scene_layer_surface->tree->node, true);
}

static void IvyLayerSurface_HandleUnmap(struct wl_listener *listener, void *data)
{
    IvyLayerSurface *layer_surface = wl_container_of(listener, layer_surface, unmap);
    (void)data;

    wlr_scene_node_set_enabled(&layer_surface->scene_layer_surface->tree->node, false);
}

static void IvyLayerSurface_HandleCommit(struct wl_listener *listener, void *data)
{
    IvyLayerSurface *layer_surface = wl_container_of(listener, layer_surface, commit);
    (void)data;

    struct wlr_layer_surface_v1 *wlr_layer_surface = layer_surface->wlr_layer_surface;
    
    if (wlr_layer_surface->initialized && wlr_layer_surface->current.committed) {
        IvyOutput *output = wlr_layer_surface->output ? wlr_layer_surface->output->data : NULL;
        if (output) {
            Ivy_Output_ArrangeLayers(output);
        }
    }
}

static void IvyLayerSurface_HandleDestroy(struct wl_listener *listener, void *data)
{
    IvyLayerSurface *layer_surface = wl_container_of(listener, layer_surface, destroy);
    (void)data;

    wl_list_remove(&layer_surface->map.link);
    wl_list_remove(&layer_surface->unmap.link);
    wl_list_remove(&layer_surface->commit.link);
    wl_list_remove(&layer_surface->destroy.link);
    wl_list_remove(&layer_surface->link);

    IvyOutput *output = layer_surface->wlr_layer_surface->output ? layer_surface->wlr_layer_surface->output->data : NULL;
    free(layer_surface);

    if (output) {
        Ivy_Output_ArrangeLayers(output);
    }
}

void Ivy_Server_HandleNewLayerSurface(struct wl_listener *listener, void *data)
{
    IvyServer *server = wl_container_of(listener, server, new_layer_surface);
    struct wlr_layer_surface_v1 *wlr_layer_surface = data;

    if (!wlr_layer_surface->output && !wl_list_empty(&server->outputs)) {
        IvyOutput *first_output = wl_container_of(server->outputs.next, first_output, link);
        wlr_layer_surface->output = first_output->wlr_output;
    }

    IvyOutput *output = wlr_layer_surface->output ? wlr_layer_surface->output->data : NULL;
    if (!output) return;

    IvyLayerSurface *layer_surface = calloc(1, sizeof(IvyLayerSurface));
    IVY_CHECK(layer_surface != NULL, "[WARNING] Failed to allocate IvyLayerSurface!");

    layer_surface->server = server;
    layer_surface->wlr_layer_surface = wlr_layer_surface;

    struct wlr_scene_tree *parent_tree;
    switch (wlr_layer_surface->pending.layer) {
        case ZWLR_LAYER_SHELL_V1_LAYER_BACKGROUND:
            parent_tree = server->scene_background;
            break;

        case ZWLR_LAYER_SHELL_V1_LAYER_BOTTOM:
            parent_tree = server->scene_bottom;
            break;

        case ZWLR_LAYER_SHELL_V1_LAYER_TOP:
            parent_tree = server->scene_top;
            break;

        case ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY:
            parent_tree = server->scene_overlay;
            break;

        default:
            parent_tree = server->scene_top;
            break;
    }

    layer_surface->scene_layer_surface = wlr_scene_layer_surface_v1_create(parent_tree, wlr_layer_surface);
    IVY_CHECK(layer_surface->scene_layer_surface != NULL, "[WARNING] Failed to create scene layer surface!");

    wlr_layer_surface->data = layer_surface;
    layer_surface->scene_layer_surface->tree->node.data = layer_surface;

    layer_surface->map.notify = IvyLayerSurface_HandleMap;
    wl_signal_add(&wlr_layer_surface->surface->events.map, &layer_surface->map);

    layer_surface->unmap.notify = IvyLayerSurface_HandleUnmap;
    wl_signal_add(&wlr_layer_surface->surface->events.unmap, &layer_surface->unmap);

    layer_surface->commit.notify = IvyLayerSurface_HandleCommit;
    wl_signal_add(&wlr_layer_surface->surface->events.commit, &layer_surface->commit);

    layer_surface->destroy.notify = IvyLayerSurface_HandleDestroy;
    wl_signal_add(&wlr_layer_surface->events.destroy, &layer_surface->destroy);

    wl_list_insert(&output->layers, &layer_surface->link);
}
