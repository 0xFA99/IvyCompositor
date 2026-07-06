#ifndef IVY_KEYBOARD_H
#define IVY_KEYBOARD_H

#include "fwd.h"

#include <wlr/types/wlr_input_device.h>

#ifdef __cplusplus
extern "C" {
#endif

struct IvyKeyboard {
    IvyServer *server;
    struct wl_list link;

    struct wlr_keyboard *wlr_keyboard;

    struct wl_listener modifiers;
    struct wl_listener key;
    struct wl_listener destroy;
};

void Ivy_Server_NewKeyboard(IvyServer *server, struct wlr_input_device *device);

#ifdef __cplusplus
}
#endif

#endif