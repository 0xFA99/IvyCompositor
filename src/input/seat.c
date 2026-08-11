#include "core/fwd.h"
#include "core/types.h"
#include "core/server.h"
#include "input/input.h"
#include "input/seat.h"
#include "input/keyboard.h"

#include <wayland-server-core.h>
#include <wlr/types/wlr_seat.h>
#include <wlr/types/wlr_data_device.h>

#include <stdlib.h>

#define IVY_SEAT_DEFAULT_NAME "seat0"

static void IvySeat_HandleRequestSetSelection(struct wl_listener *listener, void *data);

void Ivy_Seat_Init(IvySeat *seat)
{
    IVY_ASSERT(seat != NULL, "[ERROR] IvySeat is NULL!");

    IvyServer *server = wl_container_of(seat, server, input.seat);

    seat->wlr_seat = wlr_seat_create(server->core.wl_display, IVY_SEAT_DEFAULT_NAME);
    IVY_CHECK(seat->wlr_seat != NULL, "[WARNING] Failed to create wlr_seat!");

    seat->request_set_selection.notify = IvySeat_HandleRequestSetSelection;
    wl_signal_add(&seat->wlr_seat->events.request_set_selection, &seat->request_set_selection);
}

void Ivy_Seat_SetKeyboard(const IvySeat *restrict seat, const IvyKeyboard *restrict keyboard)
{
    IVY_ASSERT(seat != NULL, "[ERROR] IvySeat is NULL!");
    IVY_ASSERT(keyboard != NULL, "[ERROR] IvyKeyboard is NULL!");

    wlr_seat_set_keyboard(seat->wlr_seat, keyboard->wlr_keyboard);
}

static void IvySeat_HandleRequestSetSelection(struct wl_listener *listener, void *data)
{
    IvySeat *seat = wl_container_of(listener, seat, request_set_selection);
    const struct wlr_seat_request_set_selection_event *event = data;

    wlr_seat_set_selection(seat->wlr_seat, event->source, event->serial);
}