#include "server.h"
#include "common.h"
#include "cursor.h"
#include "output.h"

#include <wlr/xwayland.h>
#include <wlr/types/wlr_seat.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/types/wlr_keyboard.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/types/wlr_output.h>
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_output_layout.h>

#define IVY_BORDER_WIDTH 4

static const float IVY_BORDER_COLOR_FOCUSED[4] = { 0.2f, 0.6f, 0.9f, 1.0f };
static const float IVY_BORDER_COLOR_UNFOCUSED[4] = { 0.2f, 0.2f, 0.25f, 1.0f };

void IvyTopLevel_GetSize(IvyTopLevel *topLevel, int *width, int *height)
{
    if (topLevel->type == IVY_TOPLEVEL_XDG) {
        *width = topLevel->xdg_toplevel->base->geometry.width;
        *height = topLevel->xdg_toplevel->base->geometry.height;
    } else {    // XWayland
        *width = topLevel->xwayland_surface->width;
        *height = topLevel->xwayland_surface->height;
    }
}

struct wlr_output *IvyTopLevel_FindOutputForBounds(IvyServer *server, int x, int y, int width, int height)
{
    struct wlr_output *wlr_output = wlr_output_layout_output_at(server->output_layout, x + width * 0.5, y + height * 0.5);
    if (!wlr_output && !wl_list_empty(&server->outputs)) {
        IvyOutput *first_output = wl_container_of(server->outputs.next, first_output, link);
        wlr_output = first_output->wlr_output;
    }

    return wlr_output;
}

void IvyTopLevel_SetBordersEnabled(IvyTopLevel *topLevel, bool enabled)
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

void IvyTopLevel_SaveGeometry(IvyTopLevel *topLevel, const int width, const int height)
{
    topLevel->saved_geometry.x = topLevel->scene_tree->node.x;
    topLevel->saved_geometry.y = topLevel->scene_tree->node.y;
    topLevel->saved_geometry.width = width;
    topLevel->saved_geometry.height = height;
}

void IvyTopLevel_RestoreGeometry(IvyTopLevel *topLevel)
{
    const struct wlr_box *geometry = &topLevel->saved_geometry;

    wlr_scene_node_set_position(&topLevel->scene_tree->node, geometry->x, geometry->y);

    if (topLevel->type == IVY_TOPLEVEL_XDG) {
        wlr_xdg_toplevel_set_size(topLevel->xdg_toplevel, geometry->width, geometry->height);
    } else {    // XWayland
        wlr_xwayland_surface_configure(topLevel->xwayland_surface, geometry->x, geometry->y, geometry->width, geometry->height);
    }
}

void IvyTopLevel_RemoveListenerIfLinked(struct wl_listener *listener)
{
    if (listener->link.next != NULL && listener->link.next != &listener->link) {
        wl_list_remove(&listener->link);
    }
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

void Ivy_TopLevel_UpdateBorders(IvyTopLevel *topLevel)
{
    if (topLevel->scene_tree == NULL) return;

    struct wlr_surface *surface = (topLevel->type == IVY_TOPLEVEL_XDG)  ? topLevel->xdg_toplevel->base->surface
                                                                        : topLevel->xwayland_surface->surface;

    bool is_or = (topLevel->type == IVY_TOPLEVEL_XWAYLAND && topLevel->xwayland_surface->override_redirect);

    if (surface == NULL || !surface->mapped || topLevel->is_fullscreen || is_or || topLevel->is_maximized) {
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

void IvyTopLevel_HandleMap(struct wl_listener *listener, void *data)
{
    IvyTopLevel *topLevel = wl_container_of(listener, topLevel, map);
    (void)data;

    bool is_or = (topLevel->type == IVY_TOPLEVEL_XWAYLAND && topLevel->xwayland_surface->override_redirect);

    if (!is_or)
    {
        // Center the window
        struct wlr_output *wlr_output = wlr_output_layout_output_at(
            topLevel->server->output_layout,
            topLevel->server->cursor->wlr_cursor->x,
            topLevel->server->cursor->wlr_cursor->y
        );

        if (!wlr_output && !wl_list_empty(&topLevel->server->outputs)) {
            IvyOutput *first_output = wl_container_of(topLevel->server->outputs.next, first_output, link);
            wlr_output = first_output->wlr_output;
        }

        if (wlr_output) {
            struct wlr_output_layout_output *layout_output = wlr_output_layout_get(topLevel->server->output_layout, wlr_output);
            if (layout_output) {
                int width, height;
                IvyTopLevel_GetSize(topLevel, &width, &height);

                int output_width, output_height;
                wlr_output_effective_resolution(wlr_output, &output_width, &output_height);

                int geom_x = 0, geom_y = 0;
                if (topLevel->type == IVY_TOPLEVEL_XDG) {
                    geom_x = topLevel->xdg_toplevel->base->geometry.x;
                    geom_y = topLevel->xdg_toplevel->base->geometry.y;
                }

                int x = layout_output->x + (output_width - width) / 2 - geom_x;
                int y = layout_output->y + (output_height - height) / 2 - geom_y;

                wlr_scene_node_set_position(&topLevel->scene_tree->node, x, y);
            }
        }

        wl_list_insert(&topLevel->server->topLevels, &topLevel->link);
        Ivy_TopLevel_Focus(topLevel);
    }
}

void IvyTopLevel_HandleUnmap(struct wl_listener *listener, void *data)
{
    IvyTopLevel *topLevel = wl_container_of(listener, topLevel, unmap);
    (void)data;

    bool is_or = (topLevel->type == IVY_TOPLEVEL_XWAYLAND && topLevel->xwayland_surface->override_redirect);

    if (!is_or)
    {
        struct wlr_surface *focused_surface = topLevel->server->seat->keyboard_state.focused_surface;
        struct wlr_surface *surface = (topLevel->type == IVY_TOPLEVEL_XDG) ? topLevel->xdg_toplevel->base->surface
                                                                           : topLevel->xwayland_surface->surface;

        wl_list_remove(&topLevel->link);

        if (focused_surface == surface) {
            wlr_seat_keyboard_clear_focus(topLevel->server->seat);

            IvyTopLevel *next_focus = NULL;
            IvyTopLevel *tmp;

            wl_list_for_each(tmp, &topLevel->server->topLevels, link) {
                if (tmp->workspace == topLevel->server->current_workspace) {
                    next_focus = tmp;
                    break;
                }
            }

            if (next_focus != NULL) Ivy_TopLevel_Focus(next_focus);
        }
    }
}

void IvyTopLevel_HandleCommit(struct wl_listener *listener, void *data)
{
    IvyTopLevel *topLevel = wl_container_of(listener, topLevel, commit);
    (void)data;

    if (topLevel->type == IVY_TOPLEVEL_XDG) {
        if (topLevel->xdg_toplevel->base->initial_commit)
            wlr_xdg_toplevel_set_size(topLevel->xdg_toplevel, 0, 0);
    }

    Ivy_TopLevel_UpdateBorders(topLevel);
}