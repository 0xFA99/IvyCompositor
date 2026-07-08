#include "types.h"
#include "server.h"
#include "top_level.h"
#include "cursor.h"

#include <wlr/types/wlr_xcursor_manager.h>
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/types/wlr_seat.h>
#include <wlr/types/wlr_xdg_shell.h>

#include <stdlib.h>

static void IvyCursor_ProcessCursorMotion(IvyCursor *cursor, u32 time);
static void IvyCursor_ProcessCursorMove(IvyCursor *cursor);
static void IvyCursor_ProcessCursorResize(IvyCursor *cursor);

static void IvyCursor_HandleMotion(struct wl_listener *listener, void *data);
static void IvyCursor_HandleMotionAbsolute(struct wl_listener *listener, void *data);
static void IvyCursor_HandleAxis(struct wl_listener *listener, void *data);
static void IvyCursor_HandleFrame(struct wl_listener *listener, void *data);
static void IvyCursor_HandleButton(struct wl_listener *listener, void *data);

IvyCursor *Ivy_Cursor_Create(IvyServer *server)
{
    IvyCursor *cursor = calloc(1, sizeof(IvyCursor));
    IVY_CHECK(cursor != NULL, "[WARNING] Failed to allocate IvyCursor!");

    cursor->server = server;

    cursor->wlr_cursor = wlr_cursor_create();
    IVY_CHECK(cursor->wlr_cursor != NULL, "[WARNING] Failed to create wlr_cursor!");
    wlr_cursor_attach_output_layout(cursor->wlr_cursor, server->output_layout);

    cursor->cursor_mgr = wlr_xcursor_manager_create(NULL, 24);
    IVY_CHECK(cursor->cursor_mgr != NULL, "[WARNING] Failed to create xcursor manager!");
    wlr_xcursor_manager_load(cursor->cursor_mgr, 1);

    cursor->motion.notify = IvyCursor_HandleMotion;
    wl_signal_add(&cursor->wlr_cursor->events.motion, &cursor->motion);

    cursor->motion_absolute.notify = IvyCursor_HandleMotionAbsolute;
    wl_signal_add(&cursor->wlr_cursor->events.motion_absolute, &cursor->motion_absolute);

    cursor->axis.notify = IvyCursor_HandleAxis;
    wl_signal_add(&cursor->wlr_cursor->events.axis, &cursor->axis);

    cursor->frame.notify = IvyCursor_HandleFrame;
    wl_signal_add(&cursor->wlr_cursor->events.frame, &cursor->frame);

    cursor->button.notify = IvyCursor_HandleButton;
    wl_signal_add(&cursor->wlr_cursor->events.button, &cursor->button);

    return cursor;
}

void Ivy_Cursor_Destroy(IvyCursor *cursor)
{
    if (cursor == NULL) return;

    wl_list_remove(&cursor->motion.link);
    wl_list_remove(&cursor->motion_absolute.link);
    wl_list_remove(&cursor->axis.link);
    wl_list_remove(&cursor->frame.link);
    wl_list_remove(&cursor->button.link);

    wlr_xcursor_manager_destroy(cursor->cursor_mgr);
    wlr_cursor_destroy(cursor->wlr_cursor);

    free(cursor);
}

void Ivy_Server_NewPointer(IvyServer *server, struct wlr_input_device *device)
{
    wlr_cursor_attach_input_device(server->cursor->wlr_cursor, device);
}

IvyTopLevel *Ivy_Desktop_TopLevelAt(IvyServer *server, double lx, double ly, struct wlr_surface **surface, double *sx, double *sy)
{
    struct wlr_scene_node *node = wlr_scene_node_at(&server->scene->tree.node, lx, ly, sx, sy);

    if (node == NULL || node->type != WLR_SCENE_NODE_BUFFER)
        return NULL;

    struct wlr_scene_buffer *scene_buffer = wlr_scene_buffer_from_node(node);
    struct wlr_scene_surface *scene_surface = wlr_scene_surface_try_from_buffer(scene_buffer);
    if (!scene_surface) return NULL;

    *surface = scene_surface->surface;

    struct wlr_scene_tree *tree = node->parent;
    while (tree != NULL && tree->node.data == NULL)
        tree = tree->node.parent;

    return tree ? tree->node.data : NULL;
}

void Ivy_Cursor_SetSurface(IvyCursor *cursor, struct wlr_surface *surface, u32 hotspot_x, u32 hotspot_y)
{
    wlr_cursor_set_surface(cursor->wlr_cursor, surface, hotspot_x, hotspot_y);
}

void Ivy_Cursor_ResetImage(IvyCursor *cursor)
{
    wlr_cursor_set_xcursor(cursor->wlr_cursor, cursor->cursor_mgr, "default");
}

static void IvyCursor_ProcessCursorMotion(IvyCursor *cursor, u32 time)
{
    if (cursor->mode == IVY_CURSOR_MOVE) {
        IvyCursor_ProcessCursorMove(cursor);
        return;
    }

    if (cursor->mode == IVY_CURSOR_RESIZE) {
        IvyCursor_ProcessCursorResize(cursor);
        return;
    }

    // PASSTHROUGH
    double sx, sy;
    struct wlr_surface *surface = NULL;

    IvyTopLevel *topLevel = Ivy_Desktop_TopLevelAt(cursor->server, cursor->wlr_cursor->x, cursor->wlr_cursor->y, &surface, &sx, &sy);

    if (!topLevel)
        wlr_cursor_set_xcursor(cursor->wlr_cursor, cursor->cursor_mgr, "default");

    if (surface) {
        wlr_seat_pointer_notify_enter(cursor->server->seat, surface, sx, sy);
        wlr_seat_pointer_notify_motion(cursor->server->seat, time, sx, sy);
    }
    else {
        wlr_seat_pointer_clear_focus(cursor->server->seat);
    }
}

static void IvyCursor_ProcessCursorMove(IvyCursor *cursor)
{
    IvyTopLevel *topLevel = cursor->grabbed_topLevel;

    wlr_scene_node_set_position(&topLevel->scene_tree->node,
        cursor->wlr_cursor->x - cursor->grab_x,
        cursor->wlr_cursor->y - cursor->grab_y);
}

static void IvyCursor_ProcessCursorResize(IvyCursor *cursor)
{
    IvyTopLevel *topLevel = cursor->grabbed_topLevel;

    double border_x = cursor->wlr_cursor->x - cursor->grab_x;
    double border_y = cursor->wlr_cursor->y - cursor->grab_y;

    int new_left = cursor->grab_geoBox.x;
    int new_right = cursor->grab_geoBox.x + cursor->grab_geoBox.width;
    int new_top = cursor->grab_geoBox.y;
    int new_bottom = cursor->grab_geoBox.y + cursor->grab_geoBox.height;

    if (cursor->resize_edges & WLR_EDGE_TOP) {
        new_top = border_y;
        if (new_top >= new_bottom) {
            new_top = new_bottom - 1;
        }
    }
    else {
        new_bottom = border_y;
        if (new_bottom <= new_top) {
            new_bottom = new_top + 1;
        }
    }

    if (cursor->resize_edges & WLR_EDGE_LEFT) {
        new_left = border_x;
        if (new_left >= new_right) {
            new_left = new_right - 1;
        }
    }
    else if (cursor->resize_edges & WLR_EDGE_RIGHT) {
        new_right = border_x;
        if (new_right <= new_left) {
            new_right = new_left + 1;
        }
    }

    struct wlr_box *geo_box = &topLevel->xdg_toplevel->base->geometry;
    wlr_scene_node_set_position(&topLevel->scene_tree->node, new_left - geo_box->x, new_top - geo_box->y);

    int new_width = new_right - new_left;
    int new_height = new_bottom - new_top;
    wlr_xdg_toplevel_set_size(topLevel->xdg_toplevel, new_width, new_height);
}

static void IvyCursor_HandleMotion(struct wl_listener *listener, void *data)
{
    IvyCursor *cursor = wl_container_of(listener, cursor, motion);
    const struct wlr_pointer_motion_event *event = data;

    wlr_cursor_move(cursor->wlr_cursor, &event->pointer->base, event->delta_x, event->delta_y);
    IvyCursor_ProcessCursorMotion(cursor, event->time_msec);
}

static void IvyCursor_HandleMotionAbsolute(struct wl_listener *listener, void *data)
{
    IvyCursor *cursor = wl_container_of(listener, cursor, motion_absolute);
    const struct wlr_pointer_motion_absolute_event *event = data;

    wlr_cursor_warp_absolute(cursor->wlr_cursor, &event->pointer->base, event->x, event->y);
    IvyCursor_ProcessCursorMotion(cursor, event->time_msec);
}

static void IvyCursor_HandleAxis(struct wl_listener *listener, void *data)
{
    IvyCursor *cursor = wl_container_of(listener, cursor, axis);
    const struct wlr_pointer_axis_event *event = data;

    wlr_seat_pointer_notify_axis(
        cursor->server->seat, event->time_msec, event->orientation,
        event->delta, event->delta_discrete, event->source,
        event->relative_direction);
}

static void IvyCursor_HandleFrame(struct wl_listener *listener, void *data)
{
    IvyCursor *cursor = wl_container_of(listener, cursor, frame);
    (void)data;

    wlr_seat_pointer_notify_frame(cursor->server->seat);
}

static void IvyCursor_HandleButton(struct wl_listener *listener, void *data)
{
    IvyCursor *cursor = wl_container_of(listener, cursor, button);
    struct wlr_pointer_button_event *event = data;

    wlr_seat_pointer_notify_button(cursor->server->seat, event->time_msec, event->button, event->state);

    if (event->state == WL_POINTER_BUTTON_STATE_RELEASED) {
        cursor->mode = IVY_CURSOR_PASSTHROUGH;
        cursor->grabbed_topLevel = NULL;
        Ivy_Cursor_ResetImage(cursor);
    }
    else {
        double sx, sy;
        struct wlr_surface *surface = NULL;

        IvyTopLevel *topLevel = Ivy_Desktop_TopLevelAt(cursor->server, cursor->wlr_cursor->x, cursor->wlr_cursor->y, &surface, &sx, &sy);
        Ivy_TopLevel_Focus(topLevel);
    }
}

void Ivy_Cursor_BeginInteraction(IvyCursor *cursor, IvyTopLevel *topLevel, IvyCursorMode mode, u32 edges)
{
    const struct wlr_surface *focused_surface = cursor->server->seat->pointer_state.focused_surface;
    if (topLevel->xdg_toplevel->base->surface != focused_surface) return;

    cursor->grabbed_topLevel = topLevel;
    cursor->mode = mode;

    const struct wlr_xdg_surface *xdg_surface = topLevel->xdg_toplevel->base;
    struct wlr_box geo_box = xdg_surface->current.geometry;

    if (mode == IVY_CURSOR_MOVE) {
        cursor->grab_x = cursor->wlr_cursor->x - topLevel->scene_tree->node.x;
        cursor->grab_y = cursor->wlr_cursor->y - topLevel->scene_tree->node.y;
    }
    else {
        cursor->grab_x = cursor->wlr_cursor->x + geo_box.x;
        cursor->grab_y = cursor->wlr_cursor->y + geo_box.y;
    }

    cursor->grab_geoBox = geo_box;
    cursor->grab_geoBox.x += topLevel->scene_tree->node.x;
    cursor->grab_geoBox.y += topLevel->scene_tree->node.y;

    cursor->resize_edges = edges;
}