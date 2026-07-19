#include "server.h"
#include "top_level.h"

#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/types/wlr_xdg_decoration_v1.h>
#include <wlr/types/wlr_scene.h>

static void IvyTopLevel_HandleDecorationRequestMode(struct wl_listener *listener, void *data)
{
    IvyTopLevel *topLevel = wl_container_of(listener, topLevel, decoration_request_mode);
    (void)data;

    if (topLevel->xdg_decoration != NULL && topLevel->xdg_decoration->toplevel->base->initialized) {
        wlr_xdg_toplevel_decoration_v1_set_mode(topLevel->xdg_decoration, WLR_XDG_TOPLEVEL_DECORATION_V1_MODE_SERVER_SIDE);
    }
}

static void IvyTopLevel_HandleDecorationDestroy(struct wl_listener *listener, void *data)
{
    IvyTopLevel *topLevel = wl_container_of(listener, topLevel, decoration_destroy);
    (void)data;

    wl_list_remove(&topLevel->decoration_request_mode.link);
    wl_list_remove(&topLevel->decoration_destroy.link);
    topLevel->xdg_decoration = NULL;
}

void Ivy_Server_HandleNewXdgDecoration(struct wl_listener *listener, void *data)
{
    struct wlr_xdg_toplevel_decoration_v1 *decoration = data;
    (void)listener;

    struct wlr_scene_tree *tree = decoration->toplevel->base->data;
    if (tree == NULL) return;

    IvyTopLevel *topLevel = tree->node.data;
    if (topLevel == NULL) return;

    topLevel->xdg_decoration = decoration;

    topLevel->decoration_request_mode.notify = IvyTopLevel_HandleDecorationRequestMode;
    wl_signal_add(&decoration->events.request_mode, &topLevel->decoration_request_mode);

    topLevel->decoration_destroy.notify = IvyTopLevel_HandleDecorationDestroy;
    wl_signal_add(&decoration->events.destroy, &topLevel->decoration_destroy);

    if (decoration->toplevel->base->initialized) {
        wlr_xdg_toplevel_decoration_v1_set_mode(decoration, WLR_XDG_TOPLEVEL_DECORATION_V1_MODE_SERVER_SIDE);
    }
}