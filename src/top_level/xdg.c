#include "types.h"
#include "server.h"
#include "top_level.h"
#include "common.h"
#include "cursor.h"

#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/types/wlr_scene.h>

#include <stdlib.h>

static void IvyTopLevel_HandleRequestMaximize(struct wl_listener *listener, void *data);
static void IvyTopLevel_HandleRequestFullscreen(struct wl_listener *listener, void *data);
static void IvyTopLevel_HandleDestroy(struct wl_listener *listener, void *data);
static void IvyTopLevel_HandleRequestMove(struct wl_listener *listener, void *data);
static void IvyTopLevel_HandleRequestResize(struct wl_listener *listener, void *data);

void Ivy_Server_HandleNewXdgTopLevel(struct wl_listener *listener, void *data)
{
    IvyServer *server = wl_container_of(listener, server, new_xdg_topLevel);
    struct wlr_xdg_toplevel *xdg_topLevel = data;
    struct wlr_surface *wlr_surface = xdg_topLevel->base->surface;

    IvyTopLevel *topLevel = calloc(1, sizeof(IvyTopLevel));
    IVY_CHECK(topLevel != NULL, "[WARNING] Failed to allocate IvyTopLevel!");

    topLevel->server = server;
    topLevel->type = IVY_TOPLEVEL_XDG;
    topLevel->xdg_toplevel = xdg_topLevel;

    topLevel->scene_tree = wlr_scene_tree_create(server->scene_toplevel);
    IVY_CHECK(topLevel->scene_tree != NULL, "[WARNING] Failed to create scene tree for topLevel!");

    topLevel->content_tree = wlr_scene_xdg_surface_create(topLevel->scene_tree, xdg_topLevel->base);
    IVY_CHECK(topLevel->content_tree != NULL, "[WARNING] Failed to create XDG scene tree!");

    topLevel->scene_tree->node.data = topLevel;
    xdg_topLevel->base->data = topLevel->scene_tree;

    topLevel->workspace = server->current_workspace;

    topLevel->map.notify = IvyTopLevel_HandleMap;
    wl_signal_add(&wlr_surface->events.map, &topLevel->map);

    topLevel->unmap.notify = IvyTopLevel_HandleUnmap;
    wl_signal_add(&wlr_surface->events.unmap, &topLevel->unmap);

    topLevel->commit.notify = IvyTopLevel_HandleCommit;
    wl_signal_add(&wlr_surface->events.commit, &topLevel->commit);

    topLevel->destroy.notify = IvyTopLevel_HandleDestroy;
    wl_signal_add(&xdg_topLevel->events.destroy, &topLevel->destroy);

    topLevel->request_move.notify = IvyTopLevel_HandleRequestMove;
    wl_signal_add(&xdg_topLevel->events.request_move, &topLevel->request_move);

    topLevel->request_resize.notify = IvyTopLevel_HandleRequestResize;
    wl_signal_add(&xdg_topLevel->events.request_resize, &topLevel->request_resize);

    topLevel->request_maximize.notify = IvyTopLevel_HandleRequestMaximize;
    wl_signal_add(&xdg_topLevel->events.request_maximize, &topLevel->request_maximize);

    topLevel->request_fullscreen.notify = IvyTopLevel_HandleRequestFullscreen;
    wl_signal_add(&xdg_topLevel->events.request_fullscreen, &topLevel->request_fullscreen);
}

static void IvyTopLevel_HandleRequestMaximize(struct wl_listener *listener, void *data)
{
    IvyTopLevel *topLevel = wl_container_of(listener, topLevel, request_maximize);
    (void)data;

    Ivy_TopLevel_SetMaximize(topLevel, topLevel->xdg_toplevel->requested.maximized);
}

static void IvyTopLevel_HandleRequestFullscreen(struct wl_listener *listener, void *data)
{
    IvyTopLevel *topLevel = wl_container_of(listener, topLevel, request_fullscreen);
    (void)data;

    Ivy_TopLevel_SetFullscreen(topLevel, topLevel->xdg_toplevel->requested.fullscreen);
}

static void IvyTopLevel_HandleDestroy(struct wl_listener *listener, void *data)
{
    IvyTopLevel *topLevel = wl_container_of(listener, topLevel, destroy);
    (void)data;

    if (topLevel->xdg_decoration != NULL) {
        wl_list_remove(&topLevel->decoration_request_mode.link);
        wl_list_remove(&topLevel->decoration_destroy.link);
    }

    wl_list_remove(&topLevel->map.link);
    wl_list_remove(&topLevel->unmap.link);
    wl_list_remove(&topLevel->commit.link);
    wl_list_remove(&topLevel->destroy.link);

    wl_list_remove(&topLevel->request_move.link);
    wl_list_remove(&topLevel->request_resize.link);

    wl_list_remove(&topLevel->request_maximize.link);
    wl_list_remove(&topLevel->request_fullscreen.link);

    if (topLevel->link.next != NULL && topLevel->link.next != &topLevel->link) {
        wl_list_remove(&topLevel->link);
    }

    if (topLevel->scene_tree != NULL) {
        wlr_scene_node_destroy(&topLevel->scene_tree->node);
    }

    free(topLevel);
}

static void IvyTopLevel_HandleRequestMove(struct wl_listener *listener, void *data)
{
    IvyTopLevel *topLevel = wl_container_of(listener, topLevel, request_move);
    (void)data;

    Ivy_Cursor_BeginInteraction(topLevel->server->cursor, topLevel, IVY_CURSOR_MOVE, 0);
}

static void IvyTopLevel_HandleRequestResize(struct wl_listener *listener, void *data)
{
    IvyTopLevel *topLevel = wl_container_of(listener, topLevel, request_resize);
    struct wlr_xdg_toplevel_resize_event *event = data;

    Ivy_Cursor_BeginInteraction(topLevel->server->cursor, topLevel, IVY_CURSOR_RESIZE, event->edges);
}