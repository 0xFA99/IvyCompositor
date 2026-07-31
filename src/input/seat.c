#include "core/fwd.h"
#include "core/types.h"
#include "core/server.h"
#include "input/seat.h"

#include <wayland-server-core.h>
#include <wlr/backend.h>
#include <wlr/types/wlr_seat.h>
#include <wlr/types/wlr_input_device.h>

#include <stdlib.h>

#define IVY_SEAT_DEFAULT_NAME "seat0"

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
}

void Ivy_Seat_RequestCursor(struct wl_listener *listener, void *data)
{

}

void Ivy_Seat_PointerFocusChange(struct wl_listener *listener, void *data)
{

}
