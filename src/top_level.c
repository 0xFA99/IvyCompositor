#include "types.h"
#include "server.h"
#include "top_level.h"
#include "cursor.h"

#include <stdlib.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/types/wlr_seat.h>
#include <wlr/types/wlr_keyboard.h>

static void IvyTopLevel_HandleMap(struct wl_listener *listener, void *data)
{
    IvyTopLevel *topLevel = wl_container_of(listener, topLevel, map);
    (void)data;

    wl_list_insert(&topLevel->server->topLevels, &topLevel->link);

    Ivy_TopLevel_Focus(topLevel);
}

static void IvyTopLevel_HandleUnmap(struct wl_listener *listener, void *data)
{
    IvyTopLevel *topLevel = wl_container_of(listener, topLevel, unmap);
    (void)data;

    wl_list_remove(&topLevel->link);
}

static void IvyTopLevel_HandleCommit(struct wl_listener *listener, void *data)
{
    IvyTopLevel *topLevel = wl_container_of(listener, topLevel, commit);
    (void)data;

    if (topLevel->xdg_toplevel->base->initial_commit)
        wlr_xdg_toplevel_set_size(topLevel->xdg_toplevel, 0, 0);
}

static void IvyTopLevel_HandleDestroy(struct wl_listener *listener, void *data)
{
    IvyTopLevel *topLevel = wl_container_of(listener, topLevel, destroy);
    (void)data;

    wl_list_remove(&topLevel->map.link);
    wl_list_remove(&topLevel->unmap.link);
    wl_list_remove(&topLevel->commit.link);
    wl_list_remove(&topLevel->destroy.link);

    wl_list_remove(&topLevel->request_move.link);
    wl_list_remove(&topLevel->request_resize.link);

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

void Ivy_Server_HandleNewXdgTopLevel(struct wl_listener *listener, void *data)
{
    IvyServer *server = wl_container_of(listener, server, new_xdg_topLevel);
    struct wlr_xdg_toplevel *xdg_topLevel = data;
    struct wlr_surface *wlr_surface = xdg_topLevel->base->surface;

    IvyTopLevel *topLevel = calloc(1, sizeof(IvyTopLevel));
    IVY_CHECK(topLevel != NULL, "[WARNING] Failed to allocate IvyTopLevel!");

    topLevel->server = server;
    topLevel->xdg_toplevel = xdg_topLevel;
    topLevel->scene_tree = wlr_scene_xdg_surface_create(&server->scene->tree, xdg_topLevel->base);
    IVY_CHECK(topLevel->scene_tree != NULL, "[WARNING] Failed to create scene tree for topLevel!");

    topLevel->scene_tree->node.data = topLevel;
    xdg_topLevel->base->data = topLevel->scene_tree;

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
}

void Ivy_TopLevel_Focus(IvyTopLevel *topLevel)
{
    if (topLevel == NULL) return;

    IvyServer *server = topLevel->server;
    struct wlr_seat *seat = server->seat;

    struct wlr_surface *prev_surface = seat->keyboard_state.focused_surface;
    struct wlr_surface *surface = topLevel->xdg_toplevel->base->surface;

    if (prev_surface == surface) return;

    if (prev_surface != NULL) {
        struct wlr_xdg_toplevel *prev_topLevel = wlr_xdg_toplevel_try_from_wlr_surface(prev_surface);
        if (prev_topLevel != NULL) {
            wlr_xdg_toplevel_set_activated(prev_topLevel, false);
        }
    }

    struct wlr_keyboard *keyboard = wlr_seat_get_keyboard(seat);

    wlr_scene_node_raise_to_top(&topLevel->scene_tree->node);

    wl_list_remove(&topLevel->link);
    wl_list_insert(&server->topLevels, &topLevel->link);

    wlr_xdg_toplevel_set_activated(topLevel->xdg_toplevel, true);

    if (keyboard != NULL)
        wlr_seat_keyboard_notify_enter(seat, surface, keyboard->keycodes, keyboard->num_keycodes, &keyboard->modifiers);
}