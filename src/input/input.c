#include "core/fwd.h"
#include "core/server.h"
#include "core/types.h"
#include "input/input.h"
#include "input/keyboard.h"
#include "input/cursor.h"

#include <wayland-server-core.h>
#include <wlr/backend.h>
#include <wlr/types/wlr_cursor.h>

static void IvyInput_HandleNewInput(struct wl_listener *listener, void *data);
static void IvyInput_HandleNewKeyboard(IvyKeyboard *keyboard, struct wlr_input_device *input_device);
static void IvyInput_HandleNewPointer(IvyCursor *cursor, struct wlr_input_device *input_device);

void Ivy_Input_Init(IvyInput *input)
{
    IVY_ASSERT(input != NULL, "[ERROR] IvyInput is NULL!");

    IvyServer *server = wl_container_of(input, server, input);

    // keyboard manager
    Ivy_KeyboardManager_Init(&input->keyboard_manager);

    // cursor
    Ivy_Cursor_Init(&input->cursor);

    // seat
    Ivy_Seat_Init(&input->seat);

    input->new_input.notify = IvyInput_HandleNewInput;
    wl_signal_add(&server->core.wlr_backend->events.new_input, &input->new_input);
}

static void IvyInput_HandleNewInput(struct wl_listener *listener, void *data)
{
    IvyInput *input = wl_container_of(listener, input, new_input);
    struct wlr_input_device *input_device = data;

    switch (input_device->type)
    {
        case WLR_INPUT_DEVICE_KEYBOARD:
        // TODO: keyboard
        break;
        case WLR_INPUT_DEVICE_POINTER: IvyInput_HandleNewPointer(&input->cursor, input_device); break;
        default: break;
    }

    u32 caps = WL_SEAT_CAPABILITY_POINTER;
    if (!wl_list_empty(&input->keyboard_manager.keyboards)) {
        caps |= WL_SEAT_CAPABILITY_KEYBOARD;
    }

    wlr_seat_set_capabilities(input->seat.wlr_seat, caps);
}

static void IvyInput_HandleNewKeyboard(IvyKeyboard *keyboard, struct wlr_input_device *input_device)
{
    keyboard = Ivy_Keyboard_Create(input_device);
}

static void IvyInput_HandleNewPointer(IvyCursor *cursor, struct wlr_input_device *input_device)
{
    IvyInput *input = wl_container_of(cursor, input, cursor);
    wlr_cursor_attach_input_device(cursor->wlr_cursor, input_device);
}
