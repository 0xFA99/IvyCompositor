#include "core/fwd.h"
#include "core/types.h"
#include "input/seat.h"
#include "input/keyboard.h"

#include <wlr/types/wlr_keyboard.h>
#include <wlr/types/wlr_seat.h>
#include <wlr/types/wlr_input_device.h>
#include <xkbcommon/xkbcommon.h>

#include <stdlib.h>

#define IVY_KEYBOARD_DEFAULT_RATE_HZ 25
#define IVY_KEYBOARD_DEFAULT_DELAY_MS 600

static void IvyKeyboard_HandleModifiers(struct wl_listener *listener, void *data);
static void IvyKeyboard_HandleKey(struct wl_listener *listener, void *data);
static void IvyKeyboard_HandleDestroy(struct wl_listener *listener, void *data);

void Ivy_KeyboardManager_Init(IvyKeyboardManager *keyboard_manager)
{
    IVY_ASSERT(keyboard_manager != NULL, "[ERROR] IvyKeyboardManager is NULL!");

    wl_list_init(&keyboard_manager->keyboards);
}

void Ivy_KeyboardManager_Insert(IvyKeyboardManager *restrict keyboard_manager, IvyKeyboard *restrict keyboard)
{
    IVY_ASSERT(keyboard_manager != NULL, "[ERROR] IvyKeyboardManager is NULL!");
    IVY_ASSERT(keyboard != NULL, "[ERROR] IvyKeyboard is NULL!");

    wl_list_insert(&keyboard_manager->keyboards, &keyboard->link);
}

IvyKeyboard *Ivy_Keyboard_Create(struct wlr_input_device *input_device)
{
    IVY_ASSERT(input_device != NULL, "[ERROR] wlr_input_device is NULL!");

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

void Ivy_Keyboard_Init(IvySeat *restrict seat, IvyKeyboard *restrict keyboard, struct wlr_input_device *restrict input_device)
{
    IVY_ASSERT(seat != NULL, "[ERROR] IvySeat is NULL!");
    IVY_ASSERT(keyboard != NULL, "[ERROR] IvyKeyboard is NULL!");
    IVY_ASSERT(input_device != NULL, "[ERROR] wlr_input_device is NULL!");

    keyboard->seat = seat;

    keyboard->key.notify = IvyKeyboard_HandleKey;
    wl_signal_add(&keyboard->wlr_keyboard->events.key, &keyboard->key);

    keyboard->modifiers.notify = IvyKeyboard_HandleModifiers;
    wl_signal_add(&keyboard->wlr_keyboard->events.modifiers, &keyboard->modifiers);

    keyboard->destroy.notify = IvyKeyboard_HandleDestroy;
    wl_signal_add(&input_device->events.destroy, &keyboard->destroy);

    Ivy_KeyboardManager_Insert(&seat->keyboard_manager, keyboard);
    Ivy_Seat_SetKeyboard(seat, keyboard);
}

static void IvyKeyboard_HandleModifiers(struct wl_listener *listener, void *data)
{
    IvyKeyboard *keyboard = wl_container_of(listener, keyboard, modifiers);
    const IvySeat *seat = keyboard->seat;
    (void)data;

    Ivy_Seat_SetKeyboard(seat, keyboard);
    wlr_seat_keyboard_notify_modifiers(seat->wlr_seat, &keyboard->wlr_keyboard->modifiers);
}

static void IvyKeyboard_HandleKey(struct wl_listener *listener, void *data)
{
    IvyKeyboard *keyboard = wl_container_of(listener, keyboard, key);
    const IvySeat *seat = keyboard->seat;
    const struct wlr_keyboard_key_event *event = data;

    // TODO: check compositor keybinding.

    wlr_seat_set_keyboard(seat->wlr_seat, keyboard->wlr_keyboard);
    wlr_seat_keyboard_notify_key(seat->wlr_seat, event->time_msec, event->keycode, event->state);
}

static void IvyKeyboard_HandleDestroy(struct wl_listener *listener, void *data)
{
    IvyKeyboard *keyboard = wl_container_of(listener, keyboard, destroy);
    IvySeat *seat = keyboard->seat;
    (void)data;

    wl_list_remove(&keyboard->key.link);
    wl_list_remove(&keyboard->modifiers.link);
    wl_list_remove(&keyboard->destroy.link);

    wl_list_remove(&keyboard->link);

    if (seat->keyboard == keyboard) {
        seat->keyboard = NULL;
    }

    free(keyboard);

    Ivy_Seat_UpdateCapabilities(seat);
}