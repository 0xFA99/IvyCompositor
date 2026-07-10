#include "types.h"
#include "server.h"
#include "top_level.h"
#include "cursor.h"
#include "output.h"

#include <wlr/xwayland.h>
#include <wlr/types/wlr_seat.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/types/wlr_keyboard.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/types/wlr_output_layout.h>
#include <wlr/types/wlr_xdg_decoration_v1.h>

#include <stdlib.h>

#define IVY_BORDER_WIDTH 4

static const float IVY_BORDER_COLOR_FOCUSED[4] = { 0.2f, 0.6f, 0.9f, 1.0f };
static const float IVY_BORDER_COLOR_UNFOCUSED[4] = { 0.2f, 0.2f, 0.25f, 1.0f };

static void IvyTopLevel_GetSize(IvyTopLevel *topLevel, int *width, int *height);
static struct wlr_output *IvyTopLevel_FindOutputForBounds(IvyServer *server, int x, int y, int width, int height);
static void IvyTopLevel_SetBordersEnabled(IvyTopLevel *topLevel, bool enabled);
static void IvyTopLevel_SaveGeometry(IvyTopLevel *topLevel, int width, int height);
static void IvyTopLevel_RestoreGeometry(IvyTopLevel *topLevel);
static void IvyTopLevel_RemoveListenerIfLinked(struct wl_listener *listener);

static void IvyTopLevel_XWayland_HandleAssociate(struct wl_listener *listener, void *data);
static void IvyTopLevel_XWayland_HandleDissociate(struct wl_listener *listener, void *data);
static void IvyTopLevel_XWayland_HandleRequestConfigure(struct wl_listener *listener, void *data);
static void IvyTopLevel_XWayland_HandleRequestActivate(struct wl_listener *listener, void *data);
static void IvyTopLevel_XWayland_HandleRequestMaximize(struct wl_listener *listener, void *data);
static void IvyTopLevel_XWayland_HandleRequestFullscreen(struct wl_listener *listener, void *data);
static void IvyTopLevel_XWayland_HandleRequestMove(struct wl_listener *listener, void *data);
static void IvyTopLevel_XWayland_HandleRequestResize(struct wl_listener *listener, void *data);
static void IvyTopLevel_XWayland_HandleDestroy(struct wl_listener *listener, void *data);

static void IvyTopLevel_HandleDecorationRequestMode(struct wl_listener *listener, void *data);
static void IvyTopLevel_HandleDecorationDestroy(struct wl_listener *listener, void *data);
static void IvyTopLevel_HandleRequestMaximize(struct wl_listener *listener, void *data);
static void IvyTopLevel_HandleRequestFullscreen(struct wl_listener *listener, void *data);
static void IvyTopLevel_HandleMap(struct wl_listener *listener, void *data);
static void IvyTopLevel_HandleUnmap(struct wl_listener *listener, void *data);
static void IvyTopLevel_HandleCommit(struct wl_listener *listener, void *data);
static void IvyTopLevel_HandleDestroy(struct wl_listener *listener, void *data);
static void IvyTopLevel_HandleRequestMove(struct wl_listener *listener, void *data);
static void IvyTopLevel_HandleRequestResize(struct wl_listener *listener, void *data);

static void IvyTopLevel_GetSize(IvyTopLevel *topLevel, int *width, int *height)
{
    if (topLevel->type == IVY_TOPLEVEL_XDG) {
        *width = topLevel->xdg_toplevel->base->geometry.width;
        *height = topLevel->xdg_toplevel->base->geometry.height;
    } else {    // XWayland
        *width = topLevel->xwayland_surface->width;
        *height = topLevel->xwayland_surface->height;
    }
}

static struct wlr_output *IvyTopLevel_FindOutputForBounds(IvyServer *server, int x, int y, int width, int height)
{
    struct wlr_output *wlr_output = wlr_output_layout_output_at(server->output_layout, x + width * 0.5, y + height * 0.5);
    if (!wlr_output && !wl_list_empty(&server->outputs)) {
        IvyOutput *first_output = wl_container_of(server->outputs.next, first_output, link);
        wlr_output = first_output->wlr_output;
    }

    return wlr_output;
}

static void IvyTopLevel_SetBordersEnabled(IvyTopLevel *topLevel, bool enabled)
{
    struct wlr_scene_rect *borders[4] = {
        topLevel->border_top, topLevel->border_bottom,
        topLevel->border_left, topLevel->border_right
    };

    for (int i = 0; i < 4; i++) {
        if (borders[i] != NULL) {
            wlr_scene_node_set_enabled(&borders[i]->node, enabled);
        }
    }
}

static void IvyTopLevel_SaveGeometry(IvyTopLevel *topLevel, int width, int height)
{
    topLevel->saved_geometry.x = topLevel->scene_tree->node.x;
    topLevel->saved_geometry.y = topLevel->scene_tree->node.y;
    topLevel->saved_geometry.width = width;
    topLevel->saved_geometry.height = height;
}

static void IvyTopLevel_RestoreGeometry(IvyTopLevel *topLevel)
{
    const struct wlr_box *geometry = &topLevel->saved_geometry;

    wlr_scene_node_set_position(&topLevel->scene_tree->node, geometry->x, geometry->y);

    if (topLevel->type == IVY_TOPLEVEL_XDG) {
        wlr_xdg_toplevel_set_size(topLevel->xdg_toplevel, geometry->width, geometry->height);
    } else {    // XWayland
        wlr_xwayland_surface_configure(topLevel->xwayland_surface, geometry->x, geometry->y, geometry->width, geometry->height);
    }
}

static void IvyTopLevel_RemoveListenerIfLinked(struct wl_listener *listener)
{
    if (listener->link.next != NULL && listener->link.next != &listener->link) {
        wl_list_remove(&listener->link);
    }
}

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

void Ivy_TopLevel_Focus(IvyTopLevel *topLevel)
{
    if (topLevel == NULL) return;

    IvyServer *server = topLevel->server;
    struct wlr_seat *seat = server->seat;

    struct wlr_surface *prev_surface = seat->keyboard_state.focused_surface;
    struct wlr_surface *surface = (topLevel->type == IVY_TOPLEVEL_XDG)  ? topLevel->xdg_toplevel->base->surface
                                                                        : topLevel->xwayland_surface->surface;

    if (prev_surface == surface) return;

    if (prev_surface != NULL)
    {
        struct wlr_xdg_toplevel *prev_xdg = wlr_xdg_toplevel_try_from_wlr_surface(prev_surface);
        if (prev_xdg != NULL) {
            wlr_xdg_toplevel_set_activated(prev_xdg, false);
            struct wlr_scene_tree *tree = prev_xdg->base->data;
            if (tree && tree->node.data) {
                Ivy_TopLevel_UpdateBorders(tree->node.data);
            }
        }

        struct wlr_xwayland_surface *prev_xsurface = wlr_xwayland_surface_try_from_wlr_surface(prev_surface);
        if (prev_xsurface != NULL) {
            wlr_xwayland_surface_activate(prev_xsurface, false);
            if (prev_xsurface->data) {
                Ivy_TopLevel_UpdateBorders(prev_xsurface->data);
            }
        }
    }

    struct wlr_keyboard *keyboard = wlr_seat_get_keyboard(seat);

    wlr_scene_node_raise_to_top(&topLevel->scene_tree->node);

    bool is_or = (topLevel->type == IVY_TOPLEVEL_XWAYLAND && topLevel->xwayland_surface->override_redirect);
    if (!is_or) {
        wl_list_remove(&topLevel->link);
        wl_list_insert(&server->topLevels, &topLevel->link);
    }

    if (topLevel->type == IVY_TOPLEVEL_XDG) {
        wlr_xdg_toplevel_set_activated(topLevel->xdg_toplevel, true);
    }
    else {
        wlr_xwayland_surface_activate(topLevel->xwayland_surface, true);
        wlr_xwayland_surface_restack(topLevel->xwayland_surface, NULL, XCB_STACK_MODE_ABOVE);
    }

    if (keyboard != NULL && surface != NULL)
        wlr_seat_keyboard_notify_enter(seat, surface, keyboard->keycodes, keyboard->num_keycodes, &keyboard->modifiers);

    Ivy_TopLevel_UpdateBorders(topLevel);
}

void Ivy_TopLevel_SetMaximize(IvyTopLevel *topLevel, bool maximize)
{
    if (topLevel->is_maximized == maximize) return;

    IvyServer *server = topLevel->server;

    int width, height;
    IvyTopLevel_GetSize(topLevel, &width, &height);

    if (maximize)
    {
        if (!topLevel->is_fullscreen) {
            IvyTopLevel_SaveGeometry(topLevel, width, height);
        }

        struct wlr_output *wlr_output = IvyTopLevel_FindOutputForBounds(server, topLevel->scene_tree->node.x, topLevel->scene_tree->node.y, width, height);
        if (wlr_output) {
            IvyOutput *ivy_output = wlr_output->data;

            wlr_scene_node_set_position(&topLevel->scene_tree->node, ivy_output->usable_area.x, ivy_output->usable_area.y);
            if (topLevel->type == IVY_TOPLEVEL_XDG) {
                wlr_xdg_toplevel_set_size(topLevel->xdg_toplevel, ivy_output->usable_area.width, ivy_output->usable_area.height);
                wlr_xdg_toplevel_set_maximized(topLevel->xdg_toplevel, true);
            }
            else {
                wlr_xwayland_surface_configure(topLevel->xwayland_surface,
                                               ivy_output->usable_area.x, ivy_output->usable_area.y,
                                               ivy_output->usable_area.width, ivy_output->usable_area.height);

                wlr_xwayland_surface_set_maximized(topLevel->xwayland_surface, true, true);
            }

            topLevel->is_maximized = true;
        }
    }
    else {
        IvyTopLevel_RestoreGeometry(topLevel);

        if (topLevel->type == IVY_TOPLEVEL_XDG) {
            wlr_xdg_toplevel_set_maximized(topLevel->xdg_toplevel, false);
        } else {
            wlr_xwayland_surface_set_maximized(topLevel->xwayland_surface, false, false);
        }

        topLevel->is_maximized = false;
    }
}

void Ivy_TopLevel_SetFullscreen(IvyTopLevel *topLevel, bool fullscreen)
{
    if (topLevel->is_fullscreen == fullscreen) return;

    IvyServer *server = topLevel->server;

    int width, height;
    IvyTopLevel_GetSize(topLevel, &width, &height);

    if (fullscreen)
    {
        if (!topLevel->is_maximized) {
            IvyTopLevel_SaveGeometry(topLevel, width, height);
        }

        struct wlr_output *wlr_output = IvyTopLevel_FindOutputForBounds(server, topLevel->scene_tree->node.x, topLevel->scene_tree->node.y, width, height);
        if (wlr_output) {
            struct wlr_output_layout_output *layout_output = wlr_output_layout_get(server->output_layout, wlr_output);
            wlr_scene_node_set_position(&topLevel->scene_tree->node, layout_output->x, layout_output->y);

            if (topLevel->type == IVY_TOPLEVEL_XDG) {
                wlr_xdg_toplevel_set_size(topLevel->xdg_toplevel, wlr_output->width, wlr_output->height);
                wlr_xdg_toplevel_set_fullscreen(topLevel->xdg_toplevel, true);
            }
            else {
                wlr_xwayland_surface_configure(topLevel->xwayland_surface,
                                               layout_output->x, layout_output->y,
                                               wlr_output->width, wlr_output->height);

                wlr_xwayland_surface_set_fullscreen(topLevel->xwayland_surface, true);
            }
            topLevel->is_fullscreen = true;
        }
    }
    else {
        IvyTopLevel_RestoreGeometry(topLevel);

        if (topLevel->type == IVY_TOPLEVEL_XDG) {
            wlr_xdg_toplevel_set_fullscreen(topLevel->xdg_toplevel, false);
        } else {
            wlr_xwayland_surface_set_fullscreen(topLevel->xwayland_surface, false);
        }

        topLevel->is_fullscreen = false;

        if (topLevel->is_maximized) {
            topLevel->is_maximized = false;
            Ivy_TopLevel_SetMaximize(topLevel, true);
        }
    }
}

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

void Ivy_TopLevel_UpdateBorders(IvyTopLevel *topLevel)
{
    if (topLevel->scene_tree == NULL) return;

    struct wlr_surface *surface = (topLevel->type == IVY_TOPLEVEL_XDG)  ? topLevel->xdg_toplevel->base->surface
                                                                        : topLevel->xwayland_surface->surface;

    if (surface == NULL || !surface->mapped || topLevel->is_fullscreen) {
        IvyTopLevel_SetBordersEnabled(topLevel, false);
        return;
    }

    IvyTopLevel_SetBordersEnabled(topLevel, true);

    int width, height;
    IvyTopLevel_GetSize(topLevel, &width, &height);

    if (topLevel->type == IVY_TOPLEVEL_XDG) {
        struct wlr_box geometry = topLevel->xdg_toplevel->base->geometry;
        if (topLevel->content_tree != NULL) {
            wlr_scene_node_set_position(&topLevel->content_tree->node, -geometry.x, -geometry.y);
        }
    } else {
        if (topLevel->content_tree != NULL) {
            wlr_scene_node_set_position(&topLevel->content_tree->node, 0, 0);
        }
    }

    struct wlr_surface *focused_surface = topLevel->server->seat->keyboard_state.focused_surface;
    const float *border_color = (focused_surface == surface) ? IVY_BORDER_COLOR_FOCUSED
                                                             : IVY_BORDER_COLOR_UNFOCUSED;

    if (topLevel->border_top == NULL) {
        topLevel->border_top = wlr_scene_rect_create(topLevel->scene_tree, 1, 1, border_color);
        topLevel->border_bottom = wlr_scene_rect_create(topLevel->scene_tree, 1, 1, border_color);
        topLevel->border_left = wlr_scene_rect_create(topLevel->scene_tree, 1, 1, border_color);
        topLevel->border_right = wlr_scene_rect_create(topLevel->scene_tree, 1, 1, border_color);

        wlr_scene_node_lower_to_bottom(&topLevel->border_top->node);
        wlr_scene_node_lower_to_bottom(&topLevel->border_bottom->node);
        wlr_scene_node_lower_to_bottom(&topLevel->border_left->node);
        wlr_scene_node_lower_to_bottom(&topLevel->border_right->node);
    } else {
        wlr_scene_rect_set_color(topLevel->border_top, border_color);
        wlr_scene_rect_set_color(topLevel->border_bottom, border_color);
        wlr_scene_rect_set_color(topLevel->border_left, border_color);
        wlr_scene_rect_set_color(topLevel->border_right, border_color);
    }

    wlr_scene_rect_set_size(topLevel->border_top, width + 2 * IVY_BORDER_WIDTH, IVY_BORDER_WIDTH);
    wlr_scene_node_set_position(&topLevel->border_top->node, -IVY_BORDER_WIDTH, -IVY_BORDER_WIDTH);

    wlr_scene_rect_set_size(topLevel->border_bottom, width + 2 * IVY_BORDER_WIDTH, IVY_BORDER_WIDTH);
    wlr_scene_node_set_position(&topLevel->border_bottom->node, -IVY_BORDER_WIDTH, height);

    wlr_scene_rect_set_size(topLevel->border_left, IVY_BORDER_WIDTH, height);
    wlr_scene_node_set_position(&topLevel->border_left->node, -IVY_BORDER_WIDTH, 0);

    wlr_scene_rect_set_size(topLevel->border_right, IVY_BORDER_WIDTH, height);
    wlr_scene_node_set_position(&topLevel->border_right->node, width, 0);
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

static void IvyTopLevel_HandleDecorationRequestMode(struct wl_listener *listener, void *data)
{
    IvyTopLevel *topLevel = wl_container_of(listener, topLevel, decoration_request_mode);
    (void)data;

    wlr_xdg_toplevel_decoration_v1_set_mode(topLevel->xdg_decoration, WLR_XDG_TOPLEVEL_DECORATION_V1_MODE_SERVER_SIDE);
}

static void IvyTopLevel_HandleDecorationDestroy(struct wl_listener *listener, void *data)
{
    IvyTopLevel *topLevel = wl_container_of(listener, topLevel, decoration_destroy);
    (void)data;

    wl_list_remove(&topLevel->decoration_request_mode.link);
    wl_list_remove(&topLevel->decoration_destroy.link);
    topLevel->xdg_decoration = NULL;
}

void Ivy_Server_HandleNewXdgDecoration(struct wl_listener *listener, void *data)
{
    struct wlr_xdg_toplevel_decoration_v1 *decoration = data;
    (void)listener;

    struct wlr_scene_tree *tree = decoration->toplevel->base->data;
    if (tree == NULL) return;

    IvyTopLevel *topLevel = tree->node.data;
    if (topLevel == NULL) return;

    topLevel->xdg_decoration = decoration;

    topLevel->decoration_request_mode.notify = IvyTopLevel_HandleDecorationRequestMode;
    wl_signal_add(&decoration->events.request_mode, &topLevel->decoration_request_mode);

    topLevel->decoration_destroy.notify = IvyTopLevel_HandleDecorationDestroy;
    wl_signal_add(&decoration->events.destroy, &topLevel->decoration_destroy);

    wlr_xdg_toplevel_decoration_v1_set_mode(decoration, WLR_XDG_TOPLEVEL_DECORATION_V1_MODE_SERVER_SIDE);
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

static void IvyTopLevel_HandleMap(struct wl_listener *listener, void *data)
{
    IvyTopLevel *topLevel = wl_container_of(listener, topLevel, map);
    (void)data;

    bool is_or = (topLevel->type == IVY_TOPLEVEL_XWAYLAND && topLevel->xwayland_surface->override_redirect);

    if (!is_or) {
        wl_list_insert(&topLevel->server->topLevels, &topLevel->link);
        Ivy_TopLevel_Focus(topLevel);
    }
}

static void IvyTopLevel_HandleUnmap(struct wl_listener *listener, void *data)
{
    IvyTopLevel *topLevel = wl_container_of(listener, topLevel, unmap);
    (void)data;

    bool is_or = (topLevel->type == IVY_TOPLEVEL_XWAYLAND && topLevel->xwayland_surface->override_redirect);

    if (!is_or) wl_list_remove(&topLevel->link);
}

static void IvyTopLevel_HandleCommit(struct wl_listener *listener, void *data)
{
    IvyTopLevel *topLevel = wl_container_of(listener, topLevel, commit);
    (void)data;

    if (topLevel->type == IVY_TOPLEVEL_XDG) {
        if (topLevel->xdg_toplevel->base->initial_commit)
            wlr_xdg_toplevel_set_size(topLevel->xdg_toplevel, 0, 0);
    }

    Ivy_TopLevel_UpdateBorders(topLevel);
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
