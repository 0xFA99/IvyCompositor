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

    struct wl_listener key;
    struct wl_listener modifiers;
    struct wl_listener destroy;
};

struct IvyKeyboardManager {
    struct wl_list list;
};

void Ivy_KeyboardManager_Init(IvyKeyboardManager *keyboard_manager);

IvyKeyboard *Ivy_Keyboard_Create(struct wlr_input_device *input_device);

#ifdef __cplusplus
}
#endif

#endif