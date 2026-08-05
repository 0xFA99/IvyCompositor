#ifndef IVY_CORE_SEAT_H
#define IVY_CORE_SEAT_H

#include "core/fwd.h"
#include "input/keyboard.h"
#include "input/cursor.h"

#include <wayland-server-core.h>

#ifdef __cplusplus
extern "C" {
#endif

struct IvySeat {
    struct wlr_seat *wlr_seat;
    struct wl_listener new_input;

    IvyKeyboardManager keyboard_manager;
    IvyKeyboard *keyboard;  // current keyboard
    IvyCursor cursor;

    struct wl_listener request_set_selection;
};

void Ivy_Seat_Init(IvySeat *seat);
void Ivy_Seat_SetKeyboard(const IvySeat *restrict seat, const IvyKeyboard *restrict keyboard);
void Ivy_Seat_UpdateCapabilities(IvySeat *seat);

#ifdef __cplusplus
}
#endif
#endif