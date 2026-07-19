#include "types.h"
#include "server.h"
#include "top_level.h"
#include "common.h"
#include "cursor.h"

#include <wlr/xwayland.h>
#include <wlr/types/wlr_scene.h>

#include <stdlib.h>

static void IvyTopLevel_XWayland_HandleAssociate(struct wl_listener *listener, void *data);
static void IvyTopLevel_XWayland_HandleDissociate(struct wl_listener *listener, void *data);
static void IvyTopLevel_XWayland_HandleRequestConfigure(struct wl_listener *listener, void *data);
static void IvyTopLevel_XWayland_HandleRequestActivate(struct wl_listener *listener, void *data);
static void IvyTopLevel_XWayland_HandleRequestMaximize(struct wl_listener *listener, void *data);
static void IvyTopLevel_XWayland_HandleRequestFullscreen(struct wl_listener *listener, void *data);
static void IvyTopLevel_XWayland_HandleRequestMove(struct wl_listener *listener, void *data);
static void IvyTopLevel_XWayland_HandleRequestResize(struct wl_listener *listener, void *data);
static void IvyTopLevel_XWayland_HandleDestroy(struct wl_listener *listener, void *data);

void Ivy_Server_HandleNewXWaylandSurface(struct wl_listener *listener, void *data)
{
    IvyServer *server = wl_container_of(listener, server, new_xwayland_surface);
    struct wlr_xwayland_surface *xSurface = data;

    IvyTopLevel *topLevel = calloc(1, sizeof(IvyTopLevel));
    IVY_CHECK(topLevel != NULL, "[WARNING] Failed to allocate IvyTopLevel for XWayland!");

    topLevel->server = server;
    topLevel->type = IVY_TOPLEVEL_XWAYLAND;
    topLevel->xwayland_surface = xSurface;
    xSurface->data = topLevel;

    topLevel->workspace = server->current_workspace;

    wl_list_init(&topLevel->map.link);
    wl_list_init(&topLevel->unmap.link);
    wl_list_init(&topLevel->commit.link);

    topLevel->associate.notify = IvyTopLevel_XWayland_HandleAssociate;
    wl_signal_add(&xSurface->events.associate, &topLevel->associate);

    topLevel->dissociate.notify = IvyTopLevel_XWayland_HandleDissociate;
    wl_signal_add(&xSurface->events.dissociate, &topLevel->dissociate);

    topLevel->request_configure.notify = IvyTopLevel_XWayland_HandleRequestConfigure;
    wl_signal_add(&xSurface->events.request_configure, &topLevel->request_configure);

    topLevel->request_activate.notify = IvyTopLevel_XWayland_HandleRequestActivate;
    wl_signal_add(&xSurface->events.request_activate, &topLevel->request_activate);

    topLevel->request_maximize.notify = IvyTopLevel_XWayland_HandleRequestMaximize;
    wl_signal_add(&xSurface->events.request_maximize, &topLevel->request_maximize);

    topLevel->request_fullscreen.notify = IvyTopLevel_XWayland_HandleRequestFullscreen;
    wl_signal_add(&xSurface->events.request_fullscreen, &topLevel->request_fullscreen);

    topLevel->request_move.notify = IvyTopLevel_XWayland_HandleRequestMove;
    wl_signal_add(&xSurface->events.request_move, &topLevel->request_move);

    topLevel->request_resize.notify = IvyTopLevel_XWayland_HandleRequestResize;
    wl_signal_add(&xSurface->events.request_resize, &topLevel->request_resize);

    topLevel->destroy.notify = IvyTopLevel_XWayland_HandleDestroy;
    wl_signal_add(&xSurface->events.destroy, &topLevel->destroy);
}

static void IvyTopLevel_XWayland_HandleAssociate(struct wl_listener *listener, void *data)
{
    IvyTopLevel *topLevel = wl_container_of(listener, topLevel, associate);
    struct wlr_xwayland_surface *xsurface = topLevel->xwayland_surface;
    struct wlr_surface *wlr_surface = xsurface->surface;

    struct wlr_scene_tree *parent_tree = xsurface->override_redirect
        ? topLevel->server->scene_top
        : topLevel->server->scene_toplevel;

    topLevel->scene_tree = wlr_scene_tree_create(parent_tree);
    IVY_CHECK(topLevel->scene_tree != NULL, "[WARNING] Failed to create scene tree for XWayland topLevel!");

    topLevel->content_tree = wlr_scene_subsurface_tree_create(topLevel->scene_tree, wlr_surface);
    IVY_CHECK(topLevel->content_tree != NULL, "[WARNING] Failed to create XWayland subsurface tree!");

    topLevel->scene_tree->node.data = topLevel;

    topLevel->map.notify = IvyTopLevel_HandleMap;
    wl_signal_add(&wlr_surface->events.map, &topLevel->map);

    topLevel->unmap.notify = IvyTopLevel_HandleUnmap;
    wl_signal_add(&wlr_surface->events.unmap, &topLevel->unmap);

    topLevel->commit.notify = IvyTopLevel_HandleCommit;
    wl_signal_add(&wlr_surface->events.commit, &topLevel->commit);

    wlr_scene_node_set_position(&topLevel->scene_tree->node, xsurface->x, xsurface->y);
}

static void IvyTopLevel_XWayland_HandleDissociate(struct wl_listener *listener, void *data)
{
    IvyTopLevel *topLevel = wl_container_of(listener, topLevel, dissociate);
    (void)data;

    wl_list_remove(&topLevel->map.link);
    wl_list_remove(&topLevel->unmap.link);
    wl_list_remove(&topLevel->commit.link);

    if (topLevel->scene_tree) {
        wlr_scene_node_destroy(&topLevel->scene_tree->node);
        topLevel->scene_tree = NULL;
    }
}

static void IvyTopLevel_XWayland_HandleRequestConfigure(struct wl_listener *listener, void *data)
{
    IvyTopLevel *topLevel = wl_container_of(listener, topLevel, request_configure);
    struct wlr_xwayland_surface_configure_event *event = data;

    if (!topLevel->is_maximized && !topLevel->is_fullscreen) {
        wlr_xwayland_surface_configure(topLevel->xwayland_surface, event->x, event->y, event->width, event->height);
        if (topLevel->scene_tree) {
            wlr_scene_node_set_position(&topLevel->scene_tree->node, event->x, event->y);
        }
    } else {
        wlr_xwayland_surface_configure(topLevel->xwayland_surface,
            topLevel->scene_tree ? topLevel->scene_tree->node.x : event->x,
            topLevel->scene_tree ? topLevel->scene_tree->node.y : event->y,
            topLevel->xwayland_surface->width, topLevel->xwayland_surface->height);
    }
}

static void IvyTopLevel_XWayland_HandleRequestActivate(struct wl_listener *listener, void *data)
{
    IvyTopLevel *topLevel = wl_container_of(listener, topLevel, request_activate);
    (void)data;

    Ivy_TopLevel_Focus(topLevel);
}

static void IvyTopLevel_XWayland_HandleRequestMaximize(struct wl_listener *listener, void *data)
{
    IvyTopLevel *topLevel = wl_container_of(listener, topLevel, request_maximize);
    (void)data;

    const struct wlr_xwayland_surface *xSurface = topLevel->xwayland_surface;
    const bool maximize = xSurface->maximized_horz && xSurface->maximized_vert;

    Ivy_TopLevel_SetMaximize(topLevel, maximize);
}

static void IvyTopLevel_XWayland_HandleRequestFullscreen(struct wl_listener *listener, void *data)
{
    IvyTopLevel *topLevel = wl_container_of(listener, topLevel, request_fullscreen);
    (void)data;

    Ivy_TopLevel_SetFullscreen(topLevel, topLevel->xwayland_surface->fullscreen);
}

static void IvyTopLevel_XWayland_HandleRequestMove(struct wl_listener *listener, void *data)
{
    IvyTopLevel *topLevel = wl_container_of(listener, topLevel, request_move);
    (void)data;

    Ivy_Cursor_BeginInteraction(topLevel->server->cursor, topLevel, IVY_CURSOR_MOVE, 0);
}

static void IvyTopLevel_XWayland_HandleRequestResize(struct wl_listener *listener, void *data)
{
    IvyTopLevel *topLevel = wl_container_of(listener, topLevel, request_resize);
    struct wlr_xwayland_resize_event *event = data;
    Ivy_Cursor_BeginInteraction(topLevel->server->cursor, topLevel, IVY_CURSOR_RESIZE, event->edges);
}

static void IvyTopLevel_XWayland_HandleDestroy(struct wl_listener *listener, void *data)
{
    IvyTopLevel *topLevel = wl_container_of(listener, topLevel, destroy);
    (void)data;

    wl_list_remove(&topLevel->associate.link);
    wl_list_remove(&topLevel->dissociate.link);
    wl_list_remove(&topLevel->destroy.link);
    wl_list_remove(&topLevel->request_configure.link);
    wl_list_remove(&topLevel->request_activate.link);
    wl_list_remove(&topLevel->request_move.link);
    wl_list_remove(&topLevel->request_resize.link);
    wl_list_remove(&topLevel->request_maximize.link);
    wl_list_remove(&topLevel->request_fullscreen.link);

    IvyTopLevel_RemoveListenerIfLinked(&topLevel->map);
    IvyTopLevel_RemoveListenerIfLinked(&topLevel->unmap);
    IvyTopLevel_RemoveListenerIfLinked(&topLevel->commit);

    if (topLevel->link.next != NULL && topLevel->link.next != &topLevel->link) {
        wl_list_remove(&topLevel->link);
    }

    free(topLevel);
}