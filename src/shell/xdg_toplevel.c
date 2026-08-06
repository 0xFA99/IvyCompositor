#include "core/fwd.h"
#include "core/server.h"
#include "shell/xdg_toplevel.h"

#include <wayland-server-core.h>
#include <wlr/types/wlr_xdg_shell.h>

#include <stdlib.h>

static void IvyXdgTopLevelManager_HandleNewTopLevel(struct wl_listener *listener, void *data);

static void IvyXdgTopLevel_HandleMap(struct wl_listener *listener, void *data);
static void IvyXdgTopLevel_HandleUnmap(struct wl_listener *listener, void *data);
static void IvyXdgTopLevel_HandleCommit(struct wl_listener *listener, void *data);
static void IvyXdgTopLevel_HandleDestroy(struct wl_listener *listener, void *data);

void Ivy_XdgTopLevelManager_Init(IvyXdgTopLevelManager *xdg_toplevel_manager)
{
    IvyXdgShell *xdg_shell = wl_container_of(xdg_toplevel_manager, xdg_shell, xdg_toplevel_manager);

    wl_list_init(&xdg_toplevel_manager->toplevels);

    xdg_toplevel_manager->new_toplevel.notify = IvyXdgTopLevelManager_HandleNewTopLevel;
    wl_signal_add(&xdg_shell->wlr_xdg_shell->events.new_toplevel, &xdg_toplevel_manager->new_toplevel);
}

static void IvyXdgTopLevelManager_HandleNewTopLevel(struct wl_listener *listener, void *data)
{
    IvyXdgTopLevelManager *manager = wl_container_of(listener, manager, new_toplevel);
    struct wlr_xdg_toplevel *wlr_toplevel = data;

    IvyXdgTopLevel *toplevel = calloc(1, sizeof(IvyXdgTopLevel));
    IVY_CHECK(toplevel != NULL, "[WARNING] Failed to allocate IvyXdgTopLevel!");

    toplevel->wlr_xdg_toplevel = wlr_toplevel;

    toplevel->map.notify = IvyXdgTopLevel_HandleMap;
    wl_signal_add(&wlr_toplevel->base->surface->events.map, &toplevel->map);

    toplevel->unmap.notify = IvyXdgTopLevel_HandleUnmap;
    wl_signal_add(&wlr_toplevel->base->surface->events.unmap, &toplevel->unmap);

    toplevel->commit.notify = IvyXdgTopLevel_HandleCommit;
    wl_signal_add(&wlr_toplevel->base->surface->events.commit, &toplevel->commit);

    toplevel->destroy.notify = IvyXdgTopLevel_HandleDestroy;
    wl_signal_add(&wlr_toplevel->events.destroy, &toplevel->destroy);

    wl_list_insert(&manager->toplevels, &toplevel->link);
}

static void IvyXdgTopLevel_HandleMap(struct wl_listener *listener, void *data)
{
    // TODO: implement handle map.
}

static void IvyXdgTopLevel_HandleUnmap(struct wl_listener *listener, void *data)
{
    // TODO: implement handle unmap.
}

static void IvyXdgTopLevel_HandleCommit(struct wl_listener *listener, void *data)
{
    // TODO: implement handle commit.
}

static void IvyXdgTopLevel_HandleDestroy(struct wl_listener *listener, void *data)
{
    IvyXdgTopLevel *toplevel = wl_container_of(listener, toplevel, destroy);
    (void)data;

    // wl_list_remove(&toplevel->map.link);
    // wl_list_remove(&toplevel->unmap.link);
    // wl_list_remove(&toplevel->commit.link);
    wl_list_remove(&toplevel->destroy.link);

    wl_list_remove(&toplevel->link);

    free(toplevel);
}
