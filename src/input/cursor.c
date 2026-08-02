#include "core/fwd.h"
#include "core/types.h"
#include "input/seat.h"
#include "input/cursor.h"

#include <wlr/types/wlr_pointer.h>
#include <wlr/types/wlr_seat.h>

static void IvyCursor_HandleMotion(struct wl_listener *listener, void *data);
static void IvyCursor_HandleAxis(struct wl_listener *listener, void *data);
static void IvyCursor_HandleFrame(struct wl_listener *listener, void *data);
static void IvyCursor_HandleButton(struct wl_listener *listener, void *data);

static void IvyCursor_SetupListeners(IvyCursor *cursor);

void Ivy_Cursor_Init(IvyCursor *cursor)
{
    IVY_ASSERT(cursor != NULL, "[ERROR] IvyCursor is NULL!");

    cursor->wlr_cursor = wlr_cursor_create();
    IVY_CHECK(cursor->wlr_cursor != NULL, "[WARNING] Failed to create wlr_cursor!");

    IvyCursor_SetupListeners(cursor);
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

    // SEAT LISTENER ------
    cursor->request_cursor.notify = Ivy_Seat_RequestCursor;
    wl_signal_add(&seat->wlr_seat->events.request_set_cursor, &cursor->request_cursor);

    cursor->pointer_focus_change.notify = Ivy_Seat_PointerFocusChange;
    wl_signal_add(&seat->wlr_seat->pointer_state.events.focus_change, &cursor->pointer_focus_change);
}

static void IvyCursor_HandleMotion(struct wl_listener *listener, void *data)
{
    IvyCursor *cursor = wl_container_of(listener, cursor, motion);
    const struct wlr_pointer_motion_event *event = data;

    wlr_cursor_move(cursor->wlr_cursor, &event->pointer->base, event->delta_x, event->delta_y);

    switch (cursor->mode) {
        case IVY_CURSOR_PASSTHROUGH:
            // TODO: IvyCursor_ProcessPassthrough(cursor, time);
            break;

        case IVY_CURSOR_MOVE:
            // TODO: IvyCursor_ProcessMove(cursor);
            break;

        case IVY_CURSOR_RESIZE:
            // TODO: IvyCursor_ProcessResize(cursor);
            break;
    }

    wlr_idle
}

static void IvyCursor_HandleAxis(struct wl_listener *listener, void *data)
{

}

static void IvyCursor_HandleFrame(struct wl_listener *listener, void *data)
{

}

static void IvyCursor_HandleButton(struct wl_listener *listener, void *data)
{

}
