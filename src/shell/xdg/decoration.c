#include "core/fwd.h"
#include "core/types.h"
#include "core/server.h"
#include "shell/shell.h"
#include "shell/xdg/decoration.h"

#include <wayland-server-core.h>
#include <wlr/types/wlr_xdg_decoration_v1.h>

#include <stdlib.h>

static void IvyXdgDecorationManager_HandleNewToplevelDecoration(struct wl_listener *listener, void *data);
static void IvyXdgDecoration_HandleRequestMode(struct wl_listener *listener, void *data);
static void IvyXdgDecoration_HandleRequestDestroy(struct wl_listener *listener, void *data);
static void IvyXdgDecoration_HandleCommit(struct wl_listener *listener, void *data);

void Ivy_XdgDecorationManager_Init(IvyXdgDecorationManager *decoration_manager)
{
    IVY_ASSERT(decoration_manager != NULL, "[ERROR] IvyXdgDecorationManager is NULL!");

    IvyXdgShell *xdg_shell = wl_container_of(decoration_manager, xdg_shell, xdg_decoration_manager);
    IvyShell *shell = wl_container_of(xdg_shell, shell, xdg_shell);
    IvyServer *server = wl_container_of(shell, server, shell);

    decoration_manager->wlr_decoration_manager = wlr_xdg_decoration_manager_v1_create(server->core.wl_display);
    IVY_CHECK(decoration_manager->wlr_decoration_manager != NULL, "[WARNING] Failed to create wlr_xdg_decoration_manager_v1!");

    decoration_manager->new_toplevel_decoration.notify = IvyXdgDecorationManager_HandleNewToplevelDecoration;
    wl_signal_add(&decoration_manager->wlr_decoration_manager->events.new_toplevel_decoration, &decoration_manager->new_toplevel_decoration);
}

void Ivy_XdgDecorationManager_Destroy(IvyXdgDecorationManager *decoration_manager)
{
    IVY_ASSERT(decoration_manager != NULL, "[ERROR] IvyXdgDecorationManager is NULL!");

    wl_list_remove(&decoration_manager->new_toplevel_decoration.link);
}

static void IvyXdgDecorationManager_HandleNewToplevelDecoration(struct wl_listener *listener, void *data)
{
    IvyXdgDecorationManager *manager = wl_container_of(listener, manager, new_toplevel_decoration);
    struct wlr_xdg_toplevel_decoration_v1 *wlr_decoration = data;

    IvyXdgDecoration *decoration = calloc(1, sizeof(IvyXdgDecoration));
    IVY_CHECK(decoration != NULL, "[WARNING] Failed to create IvyXdgDecoration!");

    decoration->wlr_decoration = wlr_decoration;
    wlr_decoration->data = decoration;

    decoration->destroy.notify = IvyXdgDecoration_HandleRequestDestroy;
    wl_signal_add(&wlr_decoration->events.destroy, &decoration->destroy);

    decoration->request_mode.notify = IvyXdgDecoration_HandleRequestMode;
    wl_signal_add(&wlr_decoration->events.request_mode, &decoration->request_mode);

    decoration->commit.notify = IvyXdgDecoration_HandleCommit;
    wl_signal_add(&wlr_decoration->toplevel->base->surface->events.commit, &decoration->commit);
}

static void IvyXdgDecoration_HandleRequestMode(struct wl_listener *listener, void *data)
{
    IvyXdgDecoration *decoration = wl_container_of(listener, decoration, request_mode);
    (void)data;

    if (!decoration->wlr_decoration->toplevel->base->initialized)
        return;

    wlr_xdg_toplevel_decoration_v1_set_mode(decoration->wlr_decoration, WLR_XDG_TOPLEVEL_DECORATION_V1_MODE_SERVER_SIDE);
}

static void IvyXdgDecoration_HandleCommit(struct wl_listener *listener, void *data)
{
    IvyXdgDecoration *decoration = wl_container_of(listener, decoration, commit);
    (void)data;

    struct wlr_xdg_surface *xdg_surface = decoration->wlr_decoration->toplevel->base;

    if (!xdg_surface->initialized)
        return;

    wlr_xdg_toplevel_decoration_v1_set_mode(decoration->wlr_decoration, WLR_XDG_TOPLEVEL_DECORATION_V1_MODE_SERVER_SIDE);

    wl_list_remove(&decoration->commit.link);
    wl_list_init(&decoration->commit.link);
}

static void IvyXdgDecoration_HandleRequestDestroy(struct wl_listener *listener, void *data)
{
    IvyXdgDecoration *decoration = wl_container_of(listener, decoration, destroy);
    (void)data;

    wl_list_remove(&decoration->request_mode.link);
    wl_list_remove(&decoration->destroy.link);
    wl_list_remove(&decoration->commit.link);

    free(decoration);
}
