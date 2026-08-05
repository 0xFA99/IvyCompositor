#include "core/fwd.h"
#include "core/types.h"
#include "core/server.h"
#include "input/seat.h"
#include "input/cursor.h"

#include <wlr/types/wlr_seat.h>
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_pointer.h>
#include <wlr/types/wlr_xcursor_manager.h>
#include <wlr/types/wlr_scene.h>

static void IvyCursor_HandleMotion(struct wl_listener *listener, void *data);
static void IvyCursor_HandleAxis(struct wl_listener *listener, void *data);
static void IvyCursor_HandleFrame(struct wl_listener *listener, void *data);
static void IvyCursor_HandleButton(struct wl_listener *listener, void *data);

static void IvyCursor_HandleRequestCursor(struct wl_listener *listener, void *data);
static void IvyCursor_HandlePointerFocusChange(struct wl_listener *listener, void *data);

static void IvyCursor_SetupListeners(IvyCursor *cursor);

static struct wlr_surface *IvyCursor_SurfaceAt(IvyServer *server, double lx, double ly, double *sx, double *sy, IvyXdgTopLevel **out_toplevel)
{
    struct wlr_scene_node *node = wlr_scene_node_at(&server->scene.wlr_scene->tree.node, lx, ly, sx, sy);

    if (out_toplevel) {
        *out_toplevel = NULL;
    }

    if (node == NULL || node->type != WLR_SCENE_NODE_BUFFER) {
        return NULL;
    }

    struct wlr_scene_buffer *scene_buffer = wlr_scene_buffer_from_node(node);
    struct wlr_scene_surface *scene_surface = wlr_scene_surface_try_from_buffer(scene_buffer);

    if (scene_surface == NULL) {
        return NULL;
    }

    if (out_toplevel) {
        struct wlr_scene_tree *tree = node->parent;
        while (tree != NULL && tree->node.data == NULL) {
            tree = tree->node.parent;
        }
        if (tree != NULL) {
            *out_toplevel = tree->node.data;
        }
    }

    return scene_surface->surface;
}

void Ivy_Cursor_Init(IvyCursor *cursor)
{
    IVY_ASSERT(cursor != NULL, "[ERROR] IvyCursor is NULL!");

    cursor->wlr_cursor = wlr_cursor_create();
    IVY_CHECK(cursor->wlr_cursor != NULL, "[WARNING] Failed to create wlr_cursor!");

    cursor->wlr_xcursor_manager = wlr_xcursor_manager_create(NULL, IVY_CURSOR_DEFAULT_SIZE);
    IVY_CHECK(cursor->wlr_xcursor_manager != NULL, "[WARNING] Failed to create wlr_cursor!");

    wlr_xcursor_manager_load(cursor->wlr_xcursor_manager, 1);

    cursor->grab.mode = IVY_CURSOR_PASSTHROUGH;
    cursor->grab.toplevel = NULL;
    cursor->grab.x = 0;
    cursor->grab.y = 0;
    cursor->grab.resize_edges = 0;

    IvyCursor_SetupListeners(cursor);
}

void Ivy_Cursor_UpdateScale(IvyCursor *cursor, float scale)
{
    IVY_ASSERT(cursor != NULL, "[ERROR] IvyCursor is NULL!");

    wlr_xcursor_manager_load(cursor->wlr_xcursor_manager, scale);
}

static void IvyCursor_SetupListeners(IvyCursor *cursor)
{
    const IvySeat *seat = wl_container_of(cursor, seat, cursor);

    // movement
    cursor->motion.notify = IvyCursor_HandleMotion;
    wl_signal_add(&cursor->wlr_cursor->events.motion, &cursor->motion);

    // scroll
    cursor->axis.notify = IvyCursor_HandleAxis;
    wl_signal_add(&cursor->wlr_cursor->events.axis, &cursor->axis);

    cursor->frame.notify = IvyCursor_HandleFrame;
    wl_signal_add(&cursor->wlr_cursor->events.frame, &cursor->frame);

    cursor->button.notify = IvyCursor_HandleButton;
    wl_signal_add(&cursor->wlr_cursor->events.button, &cursor->button);

    cursor->request_cursor.notify = IvyCursor_HandleRequestCursor;
    wl_signal_add(&seat->wlr_seat->events.request_set_cursor, &cursor->request_cursor);

    cursor->pointer_focus_change.notify = IvyCursor_HandlePointerFocusChange;
    wl_signal_add(&seat->wlr_seat->pointer_state.events.focus_change, &cursor->pointer_focus_change);
}

static void IvyCursor_ProcessMove(IvyCursor *cursor);
static void IvyCursor_ProcessResize(IvyCursor *cursor);
static void IvyCursor_ProcessPassthrough(IvyCursor *cursor, u32 time_msec);

static void IvyCursor_HandleMotion(struct wl_listener *listener, void *data)
{
    IvyCursor *cursor = wl_container_of(listener, cursor, motion);
    const struct wlr_pointer_motion_event *event = data;

    wlr_cursor_move(cursor->wlr_cursor, &event->pointer->base, event->delta_x, event->delta_y);

    switch (cursor->grab.mode) {
        case IVY_CURSOR_PASSTHROUGH:
            IvyCursor_ProcessPassthrough(cursor, event->time_msec);
            break;

        case IVY_CURSOR_MOVE:
            IvyCursor_ProcessMove(cursor);
            break;

        case IVY_CURSOR_RESIZE:
            IvyCursor_ProcessResize(cursor);
            break;
    }
}

static void IvyCursor_ProcessPassthrough(IvyCursor *cursor, u32 time_msec)
{
    IvySeat *seat = wl_container_of(cursor, seat, cursor);
    IvyServer *server = wl_container_of(seat, server, seat);

    double sx = 0, sy = 0;
    struct wlr_surface *surface = IvyCursor_SurfaceAt(server, cursor->wlr_cursor->x, cursor->wlr_cursor->y, &sx, &sy, NULL);

    if (surface != NULL) {
        wlr_seat_pointer_notify_enter(seat->wlr_seat, surface, sx, sy);
        wlr_seat_pointer_notify_motion(seat->wlr_seat, time_msec, sx, sy);
    } else {
        wlr_seat_pointer_clear_focus(seat->wlr_seat);
        wlr_cursor_set_xcursor(cursor->wlr_cursor, cursor->wlr_xcursor_manager, IVY_CURSOR_DEFAULT_STYLE);
    }
}

static void IvyCursor_ProcessMove(IvyCursor *cursor)
{
    // TODO: implement drag-move toplevel.
    (void)cursor;
}

static void IvyCursor_ProcessResize(IvyCursor *cursor)
{
    // TODO: implement drag-resize toplevel with cursor->grab.geo_box
    (void)cursor;
}

static void IvyCursor_HandleAxis(struct wl_listener *listener, void *data)
{
    IvyCursor *cursor = wl_container_of(listener, cursor, axis);
    const struct wlr_pointer_axis_event *event = data;

    IvySeat *seat = wl_container_of(cursor, seat, cursor);

    wlr_seat_pointer_notify_axis(seat->wlr_seat, event->time_msec, event->orientation,
                                  event->delta, event->delta_discrete, event->source,
                                  event->relative_direction);
}

static void IvyCursor_HandleFrame(struct wl_listener *listener, void *data)
{
    IvyCursor *cursor = wl_container_of(listener, cursor, frame);
    IvySeat *seat = wl_container_of(cursor, seat, cursor);
    (void)data;

    wlr_seat_pointer_notify_frame(seat->wlr_seat);
}

static void IvyCursor_HandleButton(struct wl_listener *listener, void *data)
{
    IvyCursor *cursor = wl_container_of(listener, cursor, button);
    IvySeat *seat = wl_container_of(cursor, seat, cursor);
    IvyServer *server = wl_container_of(seat, server, seat);
    const struct wlr_pointer_button_event *event = data;

    wlr_seat_pointer_notify_button(seat->wlr_seat, event->time_msec, event->button, event->state);

    if (event->state == WL_POINTER_BUTTON_STATE_RELEASED) {
        if (cursor->grab.mode != IVY_CURSOR_PASSTHROUGH) {
            cursor->grab.mode = IVY_CURSOR_PASSTHROUGH;
            cursor->grab.toplevel = NULL;
            wlr_cursor_set_xcursor(cursor->wlr_cursor, cursor->wlr_xcursor_manager, IVY_CURSOR_DEFAULT_STYLE);
        }
        return;
    }

    double sx = 0, sy = 0;
    IvyXdgTopLevel *toplevel = NULL;
    struct wlr_surface *surface = IvyCursor_SurfaceAt(server, cursor->wlr_cursor->x,
                                                        cursor->wlr_cursor->y, &sx, &sy, &toplevel);

    if (surface != NULL && toplevel != NULL) {
        // TODO: Ivy_XdgTopLevel_Focus(toplevel, surface);
    }
}

static void IvyCursor_HandleRequestCursor(struct wl_listener *listener, void *data)
{
    IvyCursor *cursor = wl_container_of(listener, cursor, request_cursor);
    IvySeat *seat = wl_container_of(cursor, seat, cursor);

    const struct wlr_seat_pointer_request_set_cursor_event *event = data;
    const struct wlr_seat_client *focused_client = seat->wlr_seat->pointer_state.focused_client;

    if (focused_client == event->seat_client) {
        wlr_cursor_set_surface(cursor->wlr_cursor, event->surface, event->hotspot_x, event->hotspot_y);
    }
}

static void IvyCursor_HandlePointerFocusChange(struct wl_listener *listener, void *data)
{
    IvyCursor *cursor = wl_container_of(listener, cursor, pointer_focus_change);
    struct wlr_seat_pointer_focus_change_event *event = data;

    if (event->new_surface == NULL) {
        wlr_cursor_set_xcursor(cursor->wlr_cursor, cursor->wlr_xcursor_manager, IVY_CURSOR_DEFAULT_STYLE);
    }
}