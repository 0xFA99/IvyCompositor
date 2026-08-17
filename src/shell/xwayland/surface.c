#include "core/fwd.h"
#include "core/types.h"
#include "core/server.h"

#include "shell/xwayland/surface.h"

#include <stdlib.h>

static void IvyXwaylandSurface_HandleAssociate(struct wl_listener *listener, void *data);
static void IvyXwaylandSurface_HandleDissociate(struct wl_listener *listener, void *data);
static void IvyXwaylandSurface_HandleDestroy(struct wl_listener *listener, void *data);

static void IvyXwaylandSurface_HandleMap(struct wl_listener *listener, void *data);
static void IvyXwaylandSurface_HandleUnmap(struct wl_listener *listener, void *data);
static void IvyXwaylandSurface_HandleCommit(struct wl_listener *listener, void *data);

static void IvyXwaylandSurface_HandleRequestConfigure(struct wl_listener *listener, void *data);
static void IvyXwaylandSurface_HandleRequestMove(struct wl_listener *listener, void *data);
static void IvyXwaylandSurface_HandleRequestResize(struct wl_listener *listener, void *data);
static void IvyXwaylandSurface_HandleRequestMaximize(struct wl_listener *listener, void *data);
static void IvyXwaylandSurface_HandleRequestFullscreen(struct wl_listener *listener, void *data);
static void IvyXwaylandSurface_HandleRequestActive(struct wl_listener *listener, void *data);

IvyXwaylandSurface *Ivy_Xwayland_Create(IvyServer *server, struct wlr_xwayland_surface *wlr_xwayland_surface)
{
    IVY_ASSERT(server != NULL, "[ERROR] IvyServer is NULL!");
    IVY_ASSERT(wlr_xwayland_surface != NULL, "[ERROR] wlr_xwayland_surface is NULL!");

    IvyXwaylandSurface *surface = calloc(1, sizeof(IvyXwaylandSurface));
    IVY_CHECK(surface != NULL, "[WARNING] Failed to allocate IvyXwaylandSurface!");

    surface->wlr_xwayland_surface = wlr_xwayland_surface;
    surface->server = server;

    wlr_xwayland_surface->data = surface;

    surface->associate.notify = IvyXwaylandSurface_HandleAssociate;
    wl_signal_add(&wlr_xwayland_surface->events.associate, &surface->associate);

    surface->dissociate.notify = IvyXwaylandSurface_HandleDissociate;
    wl_signal_add(&wlr_xwayland_surface->events.dissociate, &surface->dissociate);

    surface->destroy.notify = IvyXwaylandSurface_HandleDestroy;
    wl_signal_add(&wlr_xwayland_surface->events.destroy, &surface->destroy);

    surface->request_configure.notify = IvyXwaylandSurface_HandleRequestConfigure;
    wl_signal_add(&wlr_xwayland_surface->events.request_configure, &surface->request_configure);

    surface->request_move.notify = IvyXwaylandSurface_HandleRequestMove;
    wl_signal_add(&wlr_xwayland_surface->events.request_move, &surface->request_move);

    surface->request_resize.notify = IvyXwaylandSurface_HandleRequestResize;
    wl_signal_add(&wlr_xwayland_surface->events.request_resize, &surface->request_resize);

    surface->request_maximize.notify = IvyXwaylandSurface_HandleRequestMaximize;
    wl_signal_add(&wlr_xwayland_surface->events.request_maximize, &surface->request_maximize);

    surface->request_fullscreen.notify = IvyXwaylandSurface_HandleRequestFullscreen;
    wl_signal_add(&wlr_xwayland_surface->events.request_fullscreen, &surface->request_fullscreen);

    surface->request_activate.notify = IvyXwaylandSurface_HandleRequestActive;
    wl_signal_add(&wlr_xwayland_surface->events.request_activate, &surface->request_activate);

    return surface;
}

static void IvyXwaylandSurface_HandleAssociate(struct wl_listener *listener, void *data)
{
    IvyXwaylandSurface *surface = wl_container_of(listener, surface, associate);
    struct wlr_xwayland_surface *xsurface = surface->wlr_xwayland_surface;
    (void)data;

    surface->map.notify = IvyXwaylandSurface_HandleMap;
    wl_signal_add(&xsurface->surface->events.map, &surface->map);

    surface->unmap.notify = IvyXwaylandSurface_HandleUnmap;
    wl_signal_add(&xsurface->surface->events.unmap, &surface->unmap);

    surface->commit.notify = IvyXwaylandSurface_HandleCommit;
    wl_signal_add(&xsurface->surface->events.commit, &surface->commit);
}

static void IvyXwaylandSurface_HandleDissociate(struct wl_listener *listener, void *data)
{
    IvyXwaylandSurface *surface = wl_container_of(listener, surface, dissociate);
    (void)data;

    wl_list_remove(&surface->map.link);
    wl_list_remove(&surface->unmap.link);
    wl_list_remove(&surface->commit.link);
}

static void IvyXwaylandSurface_HandleDestroy(struct wl_listener *listener, void *data)
{
    IvyXwaylandSurface *surface = wl_container_of(listener, surface, destroy);
    (void)data;

    wl_list_remove(&surface->associate.link);
    wl_list_remove(&surface->dissociate.link);
    wl_list_remove(&surface->destroy.link);
    wl_list_remove(&surface->request_configure.link);
    wl_list_remove(&surface->request_move.link);
    wl_list_remove(&surface->request_resize.link);
    wl_list_remove(&surface->request_maximize.link);
    wl_list_remove(&surface->request_fullscreen.link);
    wl_list_remove(&surface->request_activate.link);

    free(surface);
}

static void IvyXwaylandSurface_HandleMap(struct wl_listener *listener, void *data)
{

}

static void IvyXwaylandSurface_HandleUnmap(struct wl_listener *listener, void *data)
{

}

static void IvyXwaylandSurface_HandleCommit(struct wl_listener *listener, void *data)
{

}

static void IvyXwaylandSurface_HandleRequestConfigure(struct wl_listener *listener, void *data)
{

}

static void IvyXwaylandSurface_HandleRequestMove(struct wl_listener *listener, void *data)
{

}

static void IvyXwaylandSurface_HandleRequestResize(struct wl_listener *listener, void *data)
{

}

static void IvyXwaylandSurface_HandleRequestMaximize(struct wl_listener *listener, void *data)
{

}

static void IvyXwaylandSurface_HandleRequestFullscreen(struct wl_listener *listener, void *data)
{

}

static void IvyXwaylandSurface_HandleRequestActive(struct wl_listener *listener, void *data)
{

}
