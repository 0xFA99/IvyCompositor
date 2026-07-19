#include "fwd.h"
#include "layer_surface.h"
#include "server.h"
#include "types.h"
#include "popup.h"

#include <wlr/types/wlr_scene.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/types/wlr_layer_shell_v1.h>

#include <stdlib.h>

static void IvyPopup_HandleCommit(struct wl_listener *listener, void *data)
{
    IvyPopup *popup = wl_container_of(listener, popup, commit);
    (void)data;

    if (popup->xdg_popup->base->initial_commit)
        wlr_xdg_surface_schedule_configure(popup->xdg_popup->base);
}

static void IvyPopup_HandleDestroy(struct wl_listener *listener, void *data)
{
    IvyPopup *popup = wl_container_of(listener, popup, destroy);
    (void)data;

    wl_list_remove(&popup->commit.link);
    wl_list_remove(&popup->destroy.link);

    free(popup);
}

void Ivy_Server_HandleNewXdgPopup(struct wl_listener *listener, void *data)
{
    IvyServer *server = wl_container_of(listener, server, new_xdg_popup);
    struct wlr_xdg_popup *xdg_popup = data;

    IvyPopup *popup = calloc(1, sizeof(IvyPopup));
    IVY_CHECK(popup != NULL, "[WARNING] Failed to allocate IvyPopup!");

    popup->xdg_popup = xdg_popup;

    struct wlr_xdg_surface *parent = wlr_xdg_surface_try_from_wlr_surface(xdg_popup->parent);
    struct wlr_scene_tree *parent_tree = parent->data;
    struct wlr_scene_tree *popup_tree = wlr_scene_xdg_surface_create(parent_tree, xdg_popup->base);

    xdg_popup->base->data = popup_tree;

    popup->commit.notify = IvyPopup_HandleCommit;
    wl_signal_add(&xdg_popup->base->surface->events.commit, &popup->commit);

    popup->destroy.notify = IvyPopup_HandleDestroy;
    wl_signal_add(&xdg_popup->base->events.destroy, &popup->destroy);
}
