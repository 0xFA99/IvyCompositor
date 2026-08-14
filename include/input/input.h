#ifndef IVY_INPUT_H
#define IVY_INPUT_H

#include "core/fwd.h"
#include "input/seat.h"
#include "input/keyboard.h"
#include "cursor/cursor.h"
#include "cursor/shape.h"

#include <wayland-server-core.h>

#ifdef __cplusplus
extern "C" {
#endif

struct IvyInput
{
    IvySeat seat;
    struct wl_listener new_input;

    IvyKeyboardManager keyboard_manager;
    IvyCursor cursor;
    IvyCursorShape cursor_shape;
};

void Ivy_Input_Init(IvyInput *input);
void Ivy_Input_Destroy(IvyInput *input);

#ifdef __cplusplus
}
#endif

#endif