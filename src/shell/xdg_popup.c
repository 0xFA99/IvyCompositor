#include "core/fwd.h"
#include "core/types.h"
#include "core/server.h"
#include "shell/xdg_popup.h"

#include <wayland-server-core.h>
#include <wlr/types/wlr_xdg_shell.h>

#include <stdlib.h>

static void IvyXdgPopupManager_HandleNewPopup(struct wl_listener *listener, void *data);

static void IvyXdgPopup_HandleCommit(struct wl_listener *listener, void *data);
static void IvyXdgPopup_HandleDestroy(struct wl_listener *listener, void *data);

void Ivy_XdgPopupManager_Init(IvyXdgPopupManager *popup_manager)
{
    IVY_ASSERT(popup_manager != NULL, "[ERROR] IvyPopupManager is NULL!");

    IvyServer *server = wl_container_of(popup_manager, server, shell.xdg_shell.xdg_popup_manager);

    wl_list_init(&popup_manager->popups);

    popup_manager->new_popup.notify = IvyXdgPopupManager_HandleNewPopup;
    wl_signal_add(&server->shell.xdg_shell.wlr_xdg_shell->events.new_popup, &popup_manager->new_popup);
}

static void IvyXdgPopupManager_HandleNewPopup(struct wl_listener *listener, void *data)
{
    IvyXdgPopupManager *popup_manager = wl_container_of(listener, popup_manager, new_popup);
    struct wlr_xdg_popup *xdg_popup = data;

    IvyXdgPopup *popup = calloc(1, sizeof(IvyXdgPopup));
    IVY_CHECK(popup != NULL, "[WARNING] Failed to allocate IvyXdgPopup!");

    popup->xdg_popup = xdg_popup;

    struct wlr_xdg_surface *xdg_surface = wlr_xdg_surface_try_from_wlr_surface(xdg_popup->parent);
    IVY_CHECK(xdg_surface != NULL, "[WARNING] Failed to get wlr_xdg_surface!");

    struct wlr_scene_tree *xdg_surface_tree = xdg_surface->data;
    popup->xdg_popup->base->data = wlr_scene_xdg_surface_create(xdg_surface_tree, popup->xdg_popup->base);
    IVY_CHECK(popup->xdg_popup->base->data != NULL, "[WARNING] Failed to create wlr_scene_tree!");

    popup->commit.notify = IvyXdgPopup_HandleCommit;
    wl_signal_add(&xdg_popup->base->surface->events.commit, &popup->commit);

    popup->destroy.notify = IvyXdgPopup_HandleDestroy;
    wl_signal_add(&xdg_popup->events.destroy, &popup->destroy);

    wl_list_insert(&popup_manager->popups, &popup->link);
}

static void IvyXdgPopup_HandleCommit(struct wl_listener *listener, void *data)
{
    IvyXdgPopup *popup = wl_container_of(listener, popup, commit);
    (void)data;

    if (popup->xdg_popup->base->initial_commit) {
        wlr_xdg_surface_schedule_configure(popup->xdg_popup->base);
    }
}

static void IvyXdgPopup_HandleDestroy(struct wl_listener *listener, void *data)
{
    IvyXdgPopup *popup = wl_container_of(listener, popup, destroy);
    (void)data;

    wl_list_remove(&popup->commit.link);
    wl_list_remove(&popup->destroy.link);
    wl_list_remove(&popup->link);

    free(popup);
}
