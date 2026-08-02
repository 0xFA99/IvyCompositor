#ifndef IVY_INPUT_KEYBOARD_H
#define IVY_INPUT_KEYBOARD_H

#include "core/fwd.h"

#include <wayland-server-core.h>
#include <wayland-util.h>

struct wlr_input_device;

#ifdef __cplusplus
extern "C" {
#endif

struct IvyKeyboard {
    struct wlr_keyboard *wlr_keyboard;
    IvySeat *seat;

    struct wl_list link;

    struct wl_listener key;
    struct wl_listener modifiers;
    struct wl_listener destroy;
};

struct IvyKeyboardManager {
    struct wl_list keyboards;
};

void Ivy_KeyboardManager_Init(IvyKeyboardManager *keyboard_manager);

IvyKeyboard *Ivy_Keyboard_Create(struct wlr_input_device *input_device);
void Ivy_Keyboard_Init(IvySeat *restrict seat, IvyKeyboard *restrict keyboard, struct wlr_input_device *restrict input_device);

#ifdef __cplusplus
}
#endif

#endif