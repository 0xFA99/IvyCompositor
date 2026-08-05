#include "core/fwd.h"
#include "core/types.h"
#include "core/server.h"
#include "input/seat.h"

#include <wayland-server-core.h>
#include <wlr/backend.h>
#include <wlr/types/wlr_seat.h>
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_input_device.h>
#include <wlr/types/wlr_data_device.h>

#include <stdlib.h>

#define IVY_SEAT_DEFAULT_NAME "seat0"

static void IvySeat_HandleRequestSetSelection(struct wl_listener *listener, void *data);

static void IvySeat_HandleNewInput(struct wl_listener *listener, void *data)
{
    IvySeat *seat = wl_container_of(listener, seat, new_input);
    struct wlr_input_device *device = data;

    if (device->type == WLR_INPUT_DEVICE_KEYBOARD)
    {
        // TODO: Create Keyboard
    }
}

void Ivy_Seat_Init(IvySeat *seat)
{
    IVY_ASSERT(seat != NULL, "[ERROR] IvySeat is NULL!");

    const IvyServer *server = wl_container_of(seat, (IvyServer *)NULL, seat);

    seat->wlr_seat = wlr_seat_create(server->core.wl_display, IVY_SEAT_DEFAULT_NAME);
    IVY_CHECK(seat->wlr_seat != NULL, "[WARNING] Failed to create wlr_seat!");

    seat->new_input.notify = IvySeat_HandleNewInput;
    wl_signal_add(&server->core.wlr_backend->events.new_input, &seat->new_input);

    seat->request_set_selection.notify = IvySeat_HandleRequestSetSelection;
    wl_signal_add(&seat->wlr_seat->events.request_set_selection, &seat->request_set_selection);
}

void Ivy_Seat_RequestCursor(struct wl_listener *listener, void *data)
{
    IvyCursor *cursor = wl_container_of(listener, cursor, request_cursor);
    IvySeat *seat = wl_container_of(cursor, seat, cursor);

    const struct wlr_seat_pointer_request_set_cursor_event *event = data;
    const struct wlr_seat_client *focused_client = seat->wlr_seat->pointer_state.focused_client;

    if (focused_client == event->seat_client) {
        wlr_cursor_set_surface(cursor->wlr_cursor, event->surface, event->hotspot_x, event->hotspot_y);
    }
}

void Ivy_Seat_PointerFocusChange(struct wl_listener *listener, void *data)
{
    IvyCursor *cursor = wl_container_of(listener, cursor, pointer_focus_change);
    struct wlr_seat_pointer_focus_change_event *event = data;

    if (event->new_surface == NULL) {
        wlr_cursor_set_xcursor(cursor->wlr_cursor, cursor->wlr_xcursor_manager, IVY_CURSOR_DEFAULT_STYLE);
    }
}

void Ivy_Seat_SetKeyboard(const IvySeat *restrict seat, const IvyKeyboard *restrict keyboard)
{
    IVY_ASSERT(seat != NULL, "[ERROR] IvySeat is NULL!");
    IVY_ASSERT(keyboard != NULL, "[ERROR] IvyKeyboard is NULL!");

    wlr_seat_set_keyboard(seat->wlr_seat, keyboard->wlr_keyboard);
    wlr_seat_keyboard_notify_modifiers(seat->wlr_seat, &keyboard->wlr_keyboard->modifiers);
}

static void IvySeat_HandleRequestSetSelection(struct wl_listener *listener, void *data)
{
    IvySeat *seat = wl_container_of(listener, seat, request_set_selection);
    const struct wlr_seat_request_set_selection_event *event = data;

    wlr_seat_set_selection(seat->wlr_seat, event->source, event->serial);
}
