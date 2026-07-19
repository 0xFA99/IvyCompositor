#include "server.h"
#include "types.h"
#include "keyboard.h"
#include "top_level.h"

#include <wlr/types/wlr_keyboard.h>
#include <wlr/xwayland.h>
#include <wlr/types/wlr_seat.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/types/wlr_idle_notify_v1.h>
#include <xkbcommon/xkbcommon.h>

#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>

static IvyTopLevel *IvyKeyboard_GetFocusedTopLevel(IvyServer *server);
static bool IvyServer_HandleKeybinding(IvyServer *server, xkb_keysym_t sym, u32 keycode, u32 modifiers);
static void IvyKeyboard_HandleModifiers(struct wl_listener *listener, void *data);
static void IvyKeyboard_HandleKey(struct wl_listener *listener, void *data);
static void IvyKeyboard_HandleDestroy(struct wl_listener *listener, void *data);

void Ivy_Server_NewKeyboard(IvyServer *server, struct wlr_input_device *device)
{
    IVY_ASSERT(server != NULL, "[ERROR] IvyServer is NULL!");
    IVY_ASSERT(device != NULL, "[ERROR] wlr_input_device is NULL!");

    // Setup Keymap
    struct wlr_keyboard *wlr_keyboard = wlr_keyboard_from_input_device(device);

    struct xkb_context *context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
    IVY_CHECK(context != NULL, "[WARNING] Failed to create xkb_context!");

    struct xkb_keymap *keymap = xkb_keymap_new_from_names(context, NULL, XKB_KEYMAP_COMPILE_NO_FLAGS);
    IVY_CHECK(keymap != NULL, "[WARNING] Failed to create xkb_keymap!");

    wlr_keyboard_set_keymap(wlr_keyboard, keymap);
    xkb_keymap_unref(keymap);
    xkb_context_unref(context);

    wlr_keyboard_set_repeat_info(wlr_keyboard, 25, 600);

    // IvyKeyboard
    IvyKeyboard *keyboard = calloc(1, sizeof(IvyKeyboard));
    IVY_CHECK(keyboard != NULL, "[WARNING] Failed to allocate IvyKeyboard!");

    keyboard->server = server;
    keyboard->wlr_keyboard = wlr_keyboard;

    // Setup Signals
    keyboard->modifiers.notify = IvyKeyboard_HandleModifiers;
    wl_signal_add(&wlr_keyboard->events.modifiers, &keyboard->modifiers);

    keyboard->key.notify = IvyKeyboard_HandleKey;
    wl_signal_add(&wlr_keyboard->events.key, &keyboard->key);

    keyboard->destroy.notify = IvyKeyboard_HandleDestroy;
    wl_signal_add(&device->events.destroy, &keyboard->destroy);

    wlr_seat_set_keyboard(server->seat, keyboard->wlr_keyboard);
    wl_list_insert(&server->keyboards, &keyboard->link);
}

static IvyTopLevel *IvyKeyboard_GetFocusedTopLevel(IvyServer *server)
{
    struct wlr_surface *focused = server->seat->keyboard_state.focused_surface;
    if (!focused) return NULL;

    // Check XDG Shell
    struct wlr_xdg_toplevel *xdg= wlr_xdg_toplevel_try_from_wlr_surface(focused);
    if (xdg && xdg->base->data) {
        struct wlr_scene_tree *tree = xdg->base->data;
        return tree->node.data;
    }

    // Check XWayland
    struct wlr_xwayland_surface *xSurface = wlr_xwayland_surface_try_from_wlr_surface(focused);
    if (xSurface && xSurface->data)
        return xSurface->data;

    return NULL;
}

static bool IvyServer_HandleKeybinding(IvyServer *server, xkb_keysym_t sym, u32 keycode, u32 modifiers)
{
    // Workspace switching (XKB Keycode 1-9: 10-18)
    if (keycode >= 10 && keycode <= 18)
    {
        const int ws = (int)keycode - 9;

        if (modifiers & WLR_MODIFIER_SHIFT) {
            IvyTopLevel *topLevel = IvyKeyboard_GetFocusedTopLevel(server);
            if (topLevel)
                Ivy_TopLevel_MoveToWorkspace(topLevel, ws);
        } else {
            Ivy_Server_SwitchWorkspace(server, ws);
        }

        return true;
    }

    switch (sym)
    {
        case XKB_KEY_Escape:
            wl_display_terminate(server->wl_display);
            break;

        case XKB_KEY_F1:
            if (wl_list_empty(&server->topLevels)) break;

            const IvyTopLevel *focused_top = IvyKeyboard_GetFocusedTopLevel(server);
            IvyTopLevel *next_focus = NULL;

            // Circular loop focus window in workspace
            const struct wl_list *start_link = focused_top ? &focused_top->link : &server->topLevels;
            struct wl_list *element = start_link->next;

            while (element != start_link) {
                if (element == &server->topLevels) {
                    element = element->next;
                    if (element == start_link) break;
                }

                IvyTopLevel *t = wl_container_of(element, t, link);
                if (t->workspace == server->current_workspace) {
                    next_focus = t;
                    break;
                }

                element = element->next;
            }

            if (next_focus != NULL) Ivy_TopLevel_Focus(next_focus);
            break;

        case XKB_KEY_d:
            if (fork() == 0) {
                execlp("wmenu-run", "wmenu-run", NULL);
                _exit(1);
            }
            break;

        case XKB_KEY_t:
            if (fork() == 0) {
                execlp("swaylock", "swaylock", NULL);
                _exit(1);
            }
            break;

        case XKB_KEY_m: {
            IvyTopLevel *topLevel = IvyKeyboard_GetFocusedTopLevel(server);
            if (topLevel) Ivy_TopLevel_SetMaximize(topLevel, !topLevel->is_maximized);
            break;
        }

        case XKB_KEY_f: {
            IvyTopLevel *topLevel = IvyKeyboard_GetFocusedTopLevel(server);
            if (topLevel) Ivy_TopLevel_SetFullscreen(topLevel, !topLevel->is_fullscreen);
            break;
        }

        case XKB_KEY_q: {
            struct wlr_surface *focused = server->seat->keyboard_state.focused_surface;
            if (!focused) break;

            struct wlr_xdg_toplevel *xdg = wlr_xdg_toplevel_try_from_wlr_surface(focused);
            if (xdg) {
                wlr_xdg_toplevel_send_close(xdg);
            } else {
                struct wlr_xwayland_surface *xSurface = wlr_xwayland_surface_try_from_wlr_surface(focused);
                if (xSurface)
                    wlr_xwayland_surface_close(xSurface);
            }
            break;
        }

        default: return false;
    }

    return true;
}

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

    // evdev -> keycode XKB (+8)
    const u32 keycode = event->keycode + 8;

    const xkb_keysym_t *syms;
    int nSyms = xkb_state_key_get_syms(keyboard->wlr_keyboard->xkb_state, keycode, &syms);

    bool handled = false;
    u32 modifiers = wlr_keyboard_get_modifiers(keyboard->wlr_keyboard);

    wlr_idle_notifier_v1_notify_activity(keyboard->server->idle_notifier, keyboard->server->seat);

    if (modifiers & WLR_MODIFIER_ALT && event->state == WL_KEYBOARD_KEY_STATE_PRESSED)
    {
        for (int i = 0; i < nSyms; i++) {
            handled = IvyServer_HandleKeybinding(keyboard->server, syms[i], keycode, modifiers);
            if (handled) break;
        }
    }

    if (!handled) {
        wlr_seat_set_keyboard(seat, keyboard->wlr_keyboard);
        wlr_seat_keyboard_notify_key(seat, event->time_msec, event->keycode, event->state);
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
