#include "core/fwd.h"
#include "core/types.h"
#include "core/server.h"
#include "input/seat.h"
#include "input/keyboard.h"
#include "shell/xdg_toplevel.h"

#include <wlr/types/wlr_keyboard.h>
#include <wlr/types/wlr_seat.h>
#include <wlr/types/wlr_input_device.h>
#include <xkbcommon/xkbcommon.h>

#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

#define IVY_KEYBOARD_DEFAULT_RATE_HZ 25
#define IVY_KEYBOARD_DEFAULT_DELAY_MS 600

static void IvyKeyboard_HandleModifiers(struct wl_listener *listener, void *data);
static bool IvyKeyboard_HandleKeybinding(const IvyServer *server, xkb_keysym_t sym);
static void IvyKeyboard_HandleKey(struct wl_listener *listener, void *data);
static void IvyKeyboard_HandleDestroy(struct wl_listener *listener, void *data);

static void TEST_IvySpawn(const char *command)
{
    pid_t pid = fork();
    if (pid == 0) {
        if (fork() == 0) {
            setsid();
            execl("/bin/sh", "sh", "-c", command, (char *)NULL);
            _exit(EXIT_FAILURE);
        }
        _exit(EXIT_SUCCESS);
    }
    if (pid > 0) {
        waitpid(pid, NULL, 0);
    }
}

void Ivy_KeyboardManager_Init(IvyKeyboardManager *keyboard_manager)
{
    IVY_ASSERT(keyboard_manager != NULL, "[ERROR] IvyKeyboardManager is NULL!");

    wl_list_init(&keyboard_manager->keyboards);
    keyboard_manager->current_keyboard = NULL;
}

IvyKeyboard *Ivy_Keyboard_Create(IvyKeyboardManager *keyboard_manager, struct wlr_input_device *input_device)
{
    IVY_ASSERT(keyboard_manager != NULL, "[ERROR] IvyKeyboardManager is NULL!");
    IVY_ASSERT(input_device != NULL, "[ERROR] wlr_input_device is NULL!");

    IvyServer *server = wl_container_of(keyboard_manager, server, input.keyboard_manager);

    IvyKeyboard *keyboard = calloc(1, sizeof(IvyKeyboard));
    IVY_CHECK(keyboard != NULL, "[WARNING] Failed to allocate IvyKeyboard!");

    keyboard->server = server;

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

    // Signals
    keyboard->key.notify = IvyKeyboard_HandleKey;
    wl_signal_add(&keyboard->wlr_keyboard->events.key, &keyboard->key);

    keyboard->modifiers.notify = IvyKeyboard_HandleModifiers;
    wl_signal_add(&keyboard->wlr_keyboard->events.modifiers, &keyboard->modifiers);

    keyboard->destroy.notify = IvyKeyboard_HandleDestroy;
    wl_signal_add(&input_device->events.destroy, &keyboard->destroy);

    wlr_seat_set_keyboard(server->input.seat.wlr_seat, keyboard->wlr_keyboard);
    wl_list_insert(&keyboard_manager->keyboards, &keyboard->link);

    return keyboard;
}

static void IvyKeyboard_HandleModifiers(struct wl_listener *listener, void *data)
{
    IvyKeyboard *keyboard = wl_container_of(listener, keyboard, modifiers);
    const IvySeat *seat = &keyboard->server->input.seat;
    (void)data;

    wlr_seat_set_keyboard(seat->wlr_seat, keyboard->wlr_keyboard);
    wlr_seat_keyboard_notify_modifiers(seat->wlr_seat, &keyboard->wlr_keyboard->modifiers);
}

static bool IvyKeyboard_HandleKeybinding(const IvyServer *server, const xkb_keysym_t sym)
{
    switch (sym)
    {
        case XKB_KEY_Escape: {
            wl_display_terminate(server->core.wl_display);
            return true;
        }

        case XKB_KEY_u:
        case XKB_KEY_U: {
            TEST_IvySpawn("foot");
            return true;
        }

        case XKB_KEY_F1: {
            if (wl_list_length(&server->shell.xdg_shell.xdg_toplevel_manager.toplevels) < 2) {
                break;
            }

            IvyXdgTopLevel *next_toplevel = wl_container_of(server->shell.xdg_shell.xdg_toplevel_manager.toplevels.prev, next_toplevel, link);
            Ivy_XdgTopLevel_Focus(next_toplevel);
            return true;
        }
        default: break;
    }

    return false;
}

static void IvyKeyboard_HandleKey(struct wl_listener *listener, void *data)
{
    IvyKeyboard *keyboard = wl_container_of(listener, keyboard, key);
    const struct wlr_keyboard_key_event *event = data;
    const IvySeat *seat = &keyboard->server->input.seat;
    (void)data;

    const u32 keycode = event->keycode + 8;
    const xkb_keysym_t *syms;
    const int nsyms = xkb_state_key_get_syms(keyboard->wlr_keyboard->xkb_state, keycode, &syms);

    bool handled = false;
    const u32 modifiers = wlr_keyboard_get_modifiers(keyboard->wlr_keyboard);
    if (modifiers & WLR_MODIFIER_ALT && event->state == WL_KEYBOARD_KEY_STATE_PRESSED) {
        for (int i = 0; i < nsyms; i++) {
            if (IvyKeyboard_HandleKeybinding(keyboard->server, syms[i])) {
                handled = true;
                break;
            }
        }
    }

    if (!handled) {
        wlr_seat_set_keyboard(seat->wlr_seat, keyboard->wlr_keyboard);
        wlr_seat_keyboard_notify_key(seat->wlr_seat, event->time_msec, event->keycode, event->state);
    }
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

void Ivy_KeyboardManager_Destroy(IvyKeyboardManager *keyboard_manager)
{
    IVY_ASSERT(keyboard_manager != NULL, "[ERROR] IvyKeyboardManager is NULL!");

    IvyKeyboard *keyboard, *tmp;
    wl_list_for_each_safe(keyboard, tmp, &keyboard_manager->keyboards, link) {
        wl_list_remove(&keyboard->modifiers.link);
        wl_list_remove(&keyboard->key.link);
        wl_list_remove(&keyboard->destroy.link);
        wl_list_remove(&keyboard->link);
        free(keyboard);
    }
}