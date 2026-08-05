#include "core/fwd.h"
#include "core/types.h"
#include "core/server.h"
#include "input/seat.h"
#include "input/keyboard.h"
#include "input/cursor.h"

#include <wayland-server-core.h>
#include <wlr/backend.h>
#include <wlr/types/wlr_seat.h>
#include <wlr/types/wlr_input_device.h>
#include <wlr/types/wlr_data_device.h>
#include <wlr/types/wlr_cursor.h>

#include <stdlib.h>

#define IVY_SEAT_DEFAULT_NAME "seat0"

static void IvySeat_HandleNewInput(struct wl_listener *listener, void *data);
static void IvySeat_UpdateCapabilities(IvySeat *seat);
static void IvySeat_HandleRequestSetSelection(struct wl_listener *listener, void *data);

void Ivy_Seat_Init(IvySeat *seat)
{
    IVY_ASSERT(seat != NULL, "[ERROR] IvySeat is NULL!");

    IvyServer *server = wl_container_of(seat, server, seat);

    seat->wlr_seat = wlr_seat_create(server->core.wl_display, IVY_SEAT_DEFAULT_NAME);
    IVY_CHECK(seat->wlr_seat != NULL, "[WARNING] Failed to create wlr_seat!");

    Ivy_KeyboardManager_Init(&seat->keyboard_manager);
    seat->keyboard = NULL;

    Ivy_Cursor_Init(&seat->cursor);

    seat->new_input.notify = IvySeat_HandleNewInput;
    wl_signal_add(&server->core.wlr_backend->events.new_input, &seat->new_input);

    seat->request_set_selection.notify = IvySeat_HandleRequestSetSelection;
    wl_signal_add(&seat->wlr_seat->events.request_set_selection, &seat->request_set_selection);
}

void Ivy_Seat_SetKeyboard(const IvySeat *restrict seat, const IvyKeyboard *restrict keyboard)
{
    IVY_ASSERT(seat != NULL, "[ERROR] IvySeat is NULL!");
    IVY_ASSERT(keyboard != NULL, "[ERROR] IvyKeyboard is NULL!");

    wlr_seat_set_keyboard(seat->wlr_seat, keyboard->wlr_keyboard);
}

static void IvySeat_HandleNewInput(struct wl_listener *listener, void *data)
{
    IvySeat *seat = wl_container_of(listener, seat, new_input);
    struct wlr_input_device *device = data;

    switch (device->type) {
        case WLR_INPUT_DEVICE_KEYBOARD: {
            IvyKeyboard *keyboard = Ivy_Keyboard_Create(device);
            Ivy_Keyboard_Init(seat, keyboard, device);
            break;
        }

        case WLR_INPUT_DEVICE_POINTER:
        case WLR_INPUT_DEVICE_TOUCH:
            wlr_cursor_attach_input_device(seat->cursor.wlr_cursor, device);
            break;

        default:
            break;
    }

    IvySeat_UpdateCapabilities(seat);
}

static void IvySeat_UpdateCapabilities(IvySeat *seat)
{
    enum wl_seat_capability caps = WL_SEAT_CAPABILITY_POINTER;

    if (!wl_list_empty(&seat->keyboard_manager.keyboards)) {
        caps |= WL_SEAT_CAPABILITY_KEYBOARD;
    }

    wlr_seat_set_capabilities(seat->wlr_seat, caps);
}

static void IvySeat_HandleRequestSetSelection(struct wl_listener *listener, void *data)
{
    IvySeat *seat = wl_container_of(listener, seat, request_set_selection);
    const struct wlr_seat_request_set_selection_event *event = data;

    wlr_seat_set_selection(seat->wlr_seat, event->source, event->serial);
}