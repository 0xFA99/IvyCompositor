#include "core/fwd.h"
#include "core/types.h"
#include "core/server.h"
#include "input/seat.h"
#include "input/cursor.h"
#include "input/input.h"

#include <wlr/types/wlr_seat.h>
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_pointer.h>
#include <wlr/types/wlr_xcursor_manager.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/util/box.h>

#define IVY_CURSOR_DEFAULT_SIZE 24
#define IVY_CURSOR_DEFAULT_STYLE "default"

static void IvyCursor_HandleMotion(struct wl_listener *listener, void *data);
static void IvyCursor_HandleAxis(struct wl_listener *listener, void *data);
static void IvyCursor_HandleFrame(struct wl_listener *listener, void *data);
static void IvyCursor_HandleButton(struct wl_listener *listener, void *data);

static void IvyCursor_HandleRequestSetCursor(struct wl_listener *listener, void *data);
static void IvyCursor_HandlePointerFocusChange(struct wl_listener *listener, void *data);

static void IvyCursor_ProcessMove(const IvyCursor *cursor);
static void IvyCursor_ProcessResize(IvyCursor *cursor);
static void IvyCursor_ProcessPassthrough(IvyCursor *cursor, u32 time_msec);

void Ivy_Cursor_Init(IvyCursor *cursor, IvyInput *input)
{
    IVY_ASSERT(cursor != NULL, "[ERROR] IvyCursor is NULL!");
    IVY_ASSERT(input != NULL, "[ERROR] IvyInput is NULL!");

    cursor->input = input;
    IvySeat *seat = &input->seat;
    IvyServer *server = wl_container_of(seat, server, input.seat);

    cursor->wlr_cursor = wlr_cursor_create();
    IVY_CHECK(cursor->wlr_cursor != NULL, "[WARNING] Failed to create wlr_cursor!");

    wlr_cursor_attach_output_layout(cursor->wlr_cursor, server->output_manager.wlr_output_layout);

    cursor->wlr_xcursor_manager = wlr_xcursor_manager_create(NULL, IVY_CURSOR_DEFAULT_SIZE);
    wlr_xcursor_manager_load(cursor->wlr_xcursor_manager, 1.0f);

    cursor->focused_surface = NULL;
    cursor->grab.mode = IVY_CURSOR_PASSTHROUGH;

    cursor->motion.notify = IvyCursor_HandleMotion;
    wl_signal_add(&cursor->wlr_cursor->events.motion, &cursor->motion);

    cursor->button.notify = IvyCursor_HandleButton;
    wl_signal_add(&cursor->wlr_cursor->events.button, &cursor->button);

    cursor->axis.notify = IvyCursor_HandleAxis;
    wl_signal_add(&cursor->wlr_cursor->events.axis, &cursor->axis);

    cursor->frame.notify = IvyCursor_HandleFrame;
    wl_signal_add(&cursor->wlr_cursor->events.frame, &cursor->frame);

    cursor->request_cursor.notify = IvyCursor_HandleRequestSetCursor;
    wl_signal_add(&seat->wlr_seat->events.request_set_cursor, &cursor->request_cursor);

    cursor->pointer_focus_change.notify = IvyCursor_HandlePointerFocusChange;
    wl_signal_add(&seat->wlr_seat->pointer_state.events.focus_change, &cursor->pointer_focus_change);
}

static void IvyCursor_HandleMotion(struct wl_listener *listener, void *data)
{
    IvyCursor *cursor = wl_container_of(listener, cursor, motion);
    struct wlr_pointer_motion_event *event = data;

    wlr_cursor_move(cursor->wlr_cursor, &event->pointer->base, event->delta_x, event->delta_y);

    switch (cursor->grab.mode)
    {
        case IVY_CURSOR_MOVE: IvyCursor_ProcessMove(cursor); break;
        case IVY_CURSOR_RESIZE: IvyCursor_ProcessResize(cursor); break;
        case IVY_CURSOR_PASSTHROUGH: IvyCursor_ProcessPassthrough(cursor, event->time_msec); break;
        default: break;
    }
}

static void IvyCursor_ProcessPassthrough(IvyCursor *cursor, u32 time_msec)
{
    IvySeat *seat = &cursor->input->seat;
    IvyServer *server = wl_container_of(seat, server, input.seat);

    double sx, sy;
    struct wlr_surface *surface = NULL;
    IvyXdgTopLevel *toplevel = Ivy_XdgTopLevel_SurfaceAt(server, cursor->wlr_cursor->x, cursor->wlr_cursor->y, &surface, &sx, &sy);

    if (surface != cursor->focused_surface) {
        if (surface) {
            wlr_seat_pointer_notify_enter(seat->wlr_seat, surface, sx, sy);
        } else {
            wlr_seat_pointer_clear_focus(seat->wlr_seat);
        }
        cursor->focused_surface = surface;
    }

    if (surface) {
        wlr_seat_pointer_notify_motion(seat->wlr_seat, time_msec, sx, sy);
    }

    if (!toplevel) {
        wlr_cursor_set_xcursor(cursor->wlr_cursor, cursor->wlr_xcursor_manager, IVY_CURSOR_DEFAULT_STYLE);
    }
}

static void IvyCursor_ProcessMove(const IvyCursor *cursor)
{
    const IvyXdgTopLevel *toplevel = cursor->grab.toplevel;

    wlr_scene_node_set_position(&toplevel->wlr_scene_tree->node,
        (int)(cursor->wlr_cursor->x - cursor->grab.x),
        (int)(cursor->wlr_cursor->y - cursor->grab.y));
}

static void IvyCursor_ProcessResize(IvyCursor *cursor)
{
    IvyXdgTopLevel *toplevel = cursor->grab.toplevel;
    double border_x = cursor->wlr_cursor->x - cursor->grab.x;
    double border_y = cursor->wlr_cursor->y - cursor->grab.y;
    int new_left = cursor->grab.geo_box.x;
    int new_right = cursor->grab.geo_box.x + cursor->grab.geo_box.width;
    int new_top = cursor->grab.geo_box.y;
    int new_bottom = cursor->grab.geo_box.y + cursor->grab.geo_box.height;

    if (cursor->grab.resize_edges & WLR_EDGE_TOP) {
        new_top = (int)border_y;
        if (new_top >= new_bottom) {
            new_top = new_bottom - 1;
        }
    }
    else if (cursor->grab.resize_edges & WLR_EDGE_BOTTOM) {
        new_bottom = (int)border_y;
        if (new_bottom <= new_top) {
            new_bottom = new_top + 1;
        }
    }

    if (cursor->grab.resize_edges & WLR_EDGE_LEFT) {
        new_left = (int)border_x;
        if (new_left >= new_right) {
            new_left = new_right - 1;
        }
    }
    else if (cursor->grab.resize_edges & WLR_EDGE_RIGHT) {
        new_right = (int)border_x;
        if (new_right <= new_left) {
            new_right = new_left + 1;
        }
    }

    struct wlr_box *geo_box = &toplevel->wlr_xdg_toplevel->base->geometry;
    wlr_scene_node_set_position(&toplevel->wlr_scene_tree->node, new_left - geo_box->x, new_top - geo_box->y);

    int new_width = new_right - new_left;
    int new_height = new_bottom - new_top;
    wlr_xdg_toplevel_set_size(toplevel->wlr_xdg_toplevel, new_width, new_height);
}

static void IvyCursor_HandleAxis(struct wl_listener *listener, void *data)
{
    IvyCursor *cursor = wl_container_of(listener, cursor, axis);
    const struct wlr_pointer_axis_event *event = data;

    const IvySeat *seat = &cursor->input->seat;

    wlr_seat_pointer_notify_axis(seat->wlr_seat, event->time_msec, event->orientation,
                                  event->delta, event->delta_discrete, event->source,
                                  event->relative_direction);
}

static void IvyCursor_HandleFrame(struct wl_listener *listener, void *data)
{
    IvyCursor *cursor = wl_container_of(listener, cursor, frame);
    const IvySeat *seat = &cursor->input->seat;
    (void)data;

    wlr_seat_pointer_notify_frame(seat->wlr_seat);
}

static void IvyCursor_HandleButton(struct wl_listener *listener, void *data)
{
    IvyCursor *cursor = wl_container_of(listener, cursor, button);
    struct wlr_pointer_button_event *event = data;
    IvySeat *seat = &cursor->input->seat;
    IvyServer *server = wl_container_of(seat, server, input.seat);

    wlr_seat_pointer_notify_button(seat->wlr_seat, event->time_msec, event->button, event->state);

    if (event->state == WL_POINTER_BUTTON_STATE_RELEASED) {
        cursor->grab.mode = IVY_CURSOR_PASSTHROUGH;
        cursor->grab.toplevel = NULL;
    } else {
        double sx, sy;
        struct wlr_surface *surface = NULL;
        IvyXdgTopLevel *toplevel = Ivy_XdgTopLevel_SurfaceAt(server, cursor->wlr_cursor->x, cursor->wlr_cursor->y, &surface, &sx, &sy);

        Ivy_XdgTopLevel_Focus(toplevel);
    }
}

static void IvyCursor_HandleRequestSetCursor(struct wl_listener *listener, void *data)
{
    IvyCursor *cursor = wl_container_of(listener, cursor, request_cursor);
    IvySeat *seat = &cursor->input->seat;

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
    cursor->focused_surface = event->new_surface;
}