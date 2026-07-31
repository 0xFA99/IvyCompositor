#include "core/fwd.h"
#include "core/types.h"
#include "input/keyboard.h"

#include <stdlib.h>
#include <wlr/types/wlr_keyboard.h>
#include <wlr/types/wlr_seat.h>
#include <xkbcommon/xkbcommon.h>

#define IVY_KEYBOARD_DEFAULT_RATE_HZ 25
#define IVY_KEYBOARD_DEFAULT_DELAY_MS 600

static void IvyKeyboard_HandleModifiers(struct wl_listener *listener, void *data);
static void IvyKeyboard_HandleKey(struct wl_listener *listener, void *data);
static void IvyKeyboard_HandleDestroy(struct wl_listener *listener, void *data);

void Ivy_KeyboardManager_Init(IvyKeyboardManager *keyboard_manager)
{
    IVY_ASSERT(keyboard_manager != NULL, "[ERROR] IvyKeyboardManager is NULL!");

    wl_list_init(&keyboard_manager->list);
}

IvyKeyboard *Ivy_Keyboard_Create(struct wlr_input_device *input_device)
{
    IvyKeyboard *keyboard = calloc(1, sizeof(IvyKeyboard));
    IVY_CHECK(keyboard != NULL, "[WARNING] Failed to allocate IvyKeyboard!");

    keyboard->wlr_keyboard = wlr_keyboard_from_input_device(input_device);
    IVY_CHECK(keyboard->wlr_keyboard != NULL, "[WARNING] Failed to get wlr_keyboard!");

    struct xkb_context *xkb_context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
    IVY_CHECK(xkb_context != NULL, "[WARNING] Failed to create xkb_context!");

    struct xkb_keymap *xkb_keymap = xkb_keymap_new_from_names(xkb_context, NULL, XKB_KEYMAP_COMPILE_NO_FLAGS);
    IVY_CHECK(xkb_keymap != NULL, "[WARNING] Failed to create kxb_keymap!");

    wlr_keyboard_set_keymap(keyboard->wlr_keyboard, xkb_keymap);
    xkb_keymap_unref(xkb_keymap);
    xkb_context_unref(xkb_context);

    wlr_keyboard_set_repeat_info(keyboard->wlr_keyboard, IVY_KEYBOARD_DEFAULT_RATE_HZ, IVY_KEYBOARD_DEFAULT_DELAY_MS);

    return keyboard;
}

void Ivy_Keyboard_Init(IvyKeyboard *keyboard, struct wlr_input_device *input_device)
{
    keyboard->modifiers.notify = IvyKeyboard_HandleModifiers;
    wl_signal_add(&keyboard->wlr_keyboard->events.modifiers, &keyboard->modifiers);

    keyboard->key.notify = IvyKeyboard_HandleKey;
    wl_signal_add(&keyboard->wlr_keyboard->events.key, &keyboard->key);

    keyboard->destroy.notify = IvyKeyboard_HandleDestroy;
    wl_signal_add(&input_device->events.destroy, &keyboard->destroy);

    // TODO: Ivy_KeyboardManager_Insert();
    // TODO: Ivy_Keyboard_SetKeyboard();
}

void Ivy_Keyboard_SetKeyboard(IvyKeyboard *keyboard)
{
    // IvySeat *seat = wl_container_of(seat, (IvySeat *)NULL, seat);
    // wlr_seat_set_keyboard(wlr_seat, wlr_keyboard)
    // TODO: ...
}

static void IvyKeyboard_HandleModifiers(struct wl_listener *listener, void *data)
{
    // TODO: ...
}

static void IvyKeyboard_HandleKey(struct wl_listener *listener, void *data)
{
    // TODO: ...
}

static void IvyKeyboard_HandleDestroy(struct wl_listener *listener, void *data)
{
    // TODO: ...
}
