#include "core/fwd.h"
#include "core/types.h"
#include "core/server.h"
#include "shell/xdg/shell.h"
#include "shell/xdg/icon.h"

#include <wayland-server-core.h>
#include <wlr/types/wlr_xdg_toplevel_icon_v1.h>
#include <wlr/types/wlr_xdg_shell.h>

#include <stdio.h>
#include <stdlib.h>

#define IVY_XDG_TOPLEVEL_ICON_VERSION 1

#define IVY_ICON_SIZE_LIST 4
int IVY_ICON_SIZES[IVY_ICON_SIZE_LIST] = { 32, 48, 64, 128 };

static void IvyXdgToplevelIconManager_HandleSetIcon(struct wl_listener *listener, void *data);

void Ivy_XdgToplevelIconManager_Init(IvyXdgToplevelIconManager *icon_manager)
{
    IVY_ASSERT(icon_manager != NULL, "[ERROR] IvyXdgToplevelIconManager is NULL!");

    IvyXdgShell *xdg_shell = wl_container_of(icon_manager, xdg_shell, xdg_icon_manager);
    IvyShell *shell = wl_container_of(xdg_shell, shell, xdg_shell);
    IvyServer *server = wl_container_of(shell, server, shell);

    icon_manager->wlr_icon_manager = wlr_xdg_toplevel_icon_manager_v1_create(server->core.wl_display, IVY_XDG_TOPLEVEL_ICON_VERSION);
    IVY_CHECK(icon_manager != NULL, "[WARNING] Failed to create wlr_xdg_toplevel_icon_manager_v1!");

    wlr_xdg_toplevel_icon_manager_v1_set_sizes(icon_manager->wlr_icon_manager, IVY_ICON_SIZES, IVY_ICON_SIZE_LIST);

    icon_manager->set_icon.notify = IvyXdgToplevelIconManager_HandleSetIcon;
    wl_signal_add(&icon_manager->wlr_icon_manager->events.set_icon, &icon_manager->set_icon);
}
void Ivy_XdgToplevelIconManager_Destroy(IvyXdgToplevelIconManager *icon_manager)
{
    IVY_ASSERT(icon_manager != NULL, "[ERROR] IvyXdgToplevelIconManager is NULL!");

    wl_list_remove(&icon_manager->set_icon.link);
}

static void IvyXdgToplevelIconManager_HandleSetIcon(struct wl_listener *listener, void *data)
{
    (void)listener;

    struct wlr_xdg_toplevel_icon_manager_v1_set_icon_event *event = data;
    struct wlr_xdg_toplevel *toplevel = event->toplevel;
    struct wlr_xdg_toplevel_icon_v1 *icon = event->icon;

    const char *app_id = toplevel->app_id ? toplevel->app_id : "(unknown)";

    if (icon == NULL) {
        fprintf(stderr, "[icon] toplevel '%s' reset icon to default\n", app_id);
        return;
    }

    struct wlr_xdg_toplevel_icon_v1 *owned_icon = wlr_xdg_toplevel_icon_v1_ref(icon);

    struct wlr_xdg_toplevel_icon_v1_buffer *icon_buffer;
    wl_list_for_each(icon_buffer, &owned_icon->buffers, link)
    {
        fprintf(stderr, "[icon] toplevel '%s' buffer %d%d @%d\n", app_id, icon_buffer->buffer->width, icon_buffer->buffer->height, icon_buffer->scale);
    }

    // TODO: save owned_icon to IvyXdgToplevel

    wlr_xdg_toplevel_icon_v1_unref(owned_icon);
}


