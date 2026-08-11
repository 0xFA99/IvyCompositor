#ifndef IVY_INPUT_H
#define IVY_INPUT_H

#include "core/fwd.h"
#include "input/seat.h"
#include "input/keyboard.h"
#include "input/cursor.h"

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
};

void Ivy_Input_Init(IvyInput *input);

#ifdef __cplusplus
}
#endif

#endif