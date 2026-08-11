#ifndef IVY_CORE_SEAT_H
#define IVY_CORE_SEAT_H

#include "core/fwd.h"

#include <wayland-server-core.h>

#ifdef __cplusplus
extern "C" {
#endif

struct IvySeat {
    struct wlr_seat *wlr_seat;

    // cursor
    struct wl_listener request_set_cursor;
    struct wl_listener pointer_focus_change;
    struct wl_listener request_set_selection;
};

void Ivy_Seat_Init(IvySeat *seat);
void Ivy_Seat_SetKeyboard(const IvySeat *restrict seat, const IvyKeyboard *restrict keyboard);
void Ivy_Seat_UpdateCapabilities(IvySeat *seat);

#ifdef __cplusplus
}
#endif
#endif