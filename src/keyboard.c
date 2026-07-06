#include "server.h"
#include "types.h"
#include "keyboard.h"

#include <wlr/types/wlr_keyboard.h>
#include <xkbcommon/xkbcommon.h>

#include <stdlib.h>
#include <wlr/types/wlr_seat.h>

static void IvyKeyboard_HandleModifiers(struct wl_listener *listener, void *data)
{
    IvyKeyboard *keyboard = wl_container_of(listener, keyboard, modifiers);
    (void)data;

    wlr_seat_set_keyboard(keyboard->server->seat, keyboard->wlr_keyboard);
    wlr_seat_keyboard_notify_modifiers(keyboard->server->seat, &keyboard->wlr_keyboard->modifiers);
}

static void IvyKeyboard_HandleKey(struct wl_listener *listener, void *data)
{
    IvyKeyboard *keyboard = wl_container_of(listener, keyboard, key);
    struct wlr_keyboard_key_event *event = data;
    struct wlr_seat *seat = keyboard->server->seat;

    wlr_seat_set_keyboard(seat, keyboard->wlr_keyboard);
    wlr_seat_keyboard_notify_key(seat, event->time_msec, event->keycode, event->state);
}

static void IvyKeyboard_HandleDestroy(struct wl_listener *listener, void *data)
{
    IvyKeyboard *keyboard = wl_container_of(listener, keyboard, destroy);
    (void)data;

    wl_list_remove(&keyboard->modifiers.link);
    wl_list_remove(&keyboard->key.link);
    wl_list_remove(&keyboard->destroy.link);
    wl_list_remove(&keyboard->link);

    free(keyboard);
}

void Ivy_Server_NewKeyboard(IvyServer *server, struct wlr_input_device *device)
{
    IVY_ASSERT(server != NULL, "[ERROR] IvyServer is NULL!");
    IVY_ASSERT(device != NULL, "[ERROR] wlr_input_device is NULL!");

    struct wlr_keyboard *wlr_keyboard = wlr_keyboard_from_input_device(device);

    struct xkb_context *context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
    IVY_CHECK(context != NULL, "[WARNING] Failed to create xkb_context!");

    struct xkb_keymap *keymap = xkb_keymap_new_from_names(context, NULL, XKB_KEYMAP_COMPILE_NO_FLAGS);
    IVY_CHECK(keymap != NULL, "[WARNING] Failed to create xkb_keymap!");

    wlr_keyboard_set_keymap(wlr_keyboard, keymap);
    xkb_keymap_unref(keymap);
    xkb_context_unref(context);

    wlr_keyboard_set_repeat_info(wlr_keyboard, 25, 600);

    IvyKeyboard *keyboard = calloc(1, sizeof(IvyKeyboard));
    IVY_CHECK(keyboard != NULL, "[WARNING] Failed to allocate IvyKeyboard!");

    keyboard->server = server;
    keyboard->wlr_keyboard = wlr_keyboard;

    keyboard->modifiers.notify = IvyKeyboard_HandleModifiers;
    wl_signal_add(&wlr_keyboard->events.modifiers, &keyboard->modifiers);

    keyboard->key.notify = IvyKeyboard_HandleKey;
    wl_signal_add(&wlr_keyboard->events.key, &keyboard->key);

    keyboard->destroy.notify = IvyKeyboard_HandleDestroy;
    wl_signal_add(&device->events.destroy, &keyboard->destroy);

    wlr_seat_set_keyboard(server->seat, keyboard->wlr_keyboard);
    wl_list_insert(&server->keyboards, &keyboard->link);

}
