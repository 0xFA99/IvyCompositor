#include "types.h"
#include "fwd.h"
#include "server.h"
#include "input.h"
#include "keyboard.h"

#include <wayland-server-protocol.h>
#include <wlr/types/wlr_input_device.h>
#include <wlr/types/wlr_seat.h>

void Ivy_Server_HandleNewInput(struct wl_listener *listener, void *data)
{
    IvyServer *server = wl_container_of(listener, server, new_input);
    struct wlr_input_device *device = data;

    if (device->type == WLR_INPUT_DEVICE_KEYBOARD)
        Ivy_Server_NewKeyboard(server, device);

    u32 caps = 0;
    if (!wl_list_empty(&server->keyboards))
        caps |= WL_SEAT_CAPABILITY_KEYBOARD;

    wlr_seat_set_capabilities(server->seat, caps);
}
