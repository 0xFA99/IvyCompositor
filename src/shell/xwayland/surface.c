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
    IvyXwaylandSurface *surface = wl_container_of(listener, surface, map);
    struct wlr_xwayland_surface *xsurface = surface->wlr_xwayland_surface;
    (void)data;

    IvyServer *server = surface->server;

    struct wlr_scene_tree *parent_tree = xsurface->override_redirect ? server->scene.top : server->scene.toplevel;
    surface->wlr_scene_tree = wlr_scene_tree_create(parent_tree);
    wlr_scene_surface_create(surface->wlr_scene_tree, xsurface->surface);
    surface->wlr_scene_tree->node.data = surface;

    wlr_scene_node_set_position(&surface->wlr_scene_tree->node, xsurface->x, xsurface->y);
    wl_list_insert(&server->shell.xwayland.surface, &surface->link);

    if (!xsurface->override_redirect) {
        Ivy_Xwayland_Focus(surface);
    } else if (wlr_xwayland_surface_override_redirect_wants_focus(xsurface))
    {
        Ivy_Xwayland_Focus(surface);
    }
}

static void IvyXwaylandSurface_HandleUnmap(struct wl_listener *listener, void *data)
{
    IvyXwaylandSurface *surface = wl_container_of(listener, surface, unmap);
    (void)data;

    wl_list_remove(&surface->link);

    if (surface->wlr_scene_tree != NULL) {
        wlr_scene_node_destroy(&surface->wlr_scene_tree->node);
        surface->wlr_scene_tree = NULL;
    }
}

static void IvyXwaylandSurface_HandleCommit(struct wl_listener *listener, void *data)
{
    IvyXwaylandSurface *surface = wl_container_of(listener, surface, commit);
    struct wlr_xwayland_surface *xsurface = surface->wlr_xwayland_surface;
    (void)data;

    if (surface->wlr_scene_tree != NULL) {
        wlr_scene_node_set_position(&surface->wlr_scene_tree->node, xsurface->x, xsurface->y);
    }
}

static void IvyXwaylandSurface_HandleRequestConfigure(struct wl_listener *listener, void *data)
{
    IvyXwaylandSurface *surface = wl_container_of(listener, surface, request_configure);
    struct wlr_xwayland_surface_configure_event *event = data;

    wlr_xwayland_surface_configure(surface->wlr_xwayland_surface, event->x, event->y, event->width, event->height);

    if (surface->wlr_scene_tree != NULL) {
        wlr_scene_node_set_position(&surface->wlr_scene_tree->node, event->x, event->y);
    }
}

static void IvyXwaylandSurface_HandleRequestMove(struct wl_listener *listener, void *data)
{
    IvyXwaylandSurface *surface = wl_container_of(listener, surface, request_move);
    (void)data;

    // TODO: implement interactive move
}

static void IvyXwaylandSurface_HandleRequestResize(struct wl_listener *listener, void *data)
{
    IvyXwaylandSurface *surface = wl_container_of(listener, surface, request_resize);
    (void)data;

    // TODO: implement interactive resize
}

static void IvyXwaylandSurface_HandleRequestMaximize(struct wl_listener *listener, void *data)
{
    IvyXwaylandSurface *surface = wl_container_of(listener, surface, request_maximize);
    (void)data;

    wlr_xwayland_surface_set_maximized(
        surface->wlr_xwayland_surface,
        surface->wlr_xwayland_surface->maximized_horz,
        surface->wlr_xwayland_surface->maximized_vert);
}

static void IvyXwaylandSurface_HandleRequestFullscreen(struct wl_listener *listener, void *data)
{
    IvyXwaylandSurface *surface = wl_container_of(listener, surface, request_fullscreen);
    (void)data;

    wlr_xwayland_surface_set_fullscreen(surface->wlr_xwayland_surface, surface->wlr_xwayland_surface->fullscreen);
}

static void IvyXwaylandSurface_HandleRequestActive(struct wl_listener *listener, void *data)
{
    IvyXwaylandSurface *surface = wl_container_of(listener, surface, request_activate);
    (void)data;

    Ivy_Xwayland_Focus(surface);
}