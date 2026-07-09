#include "server.h"
#include "types.h"
#include "keyboard.h"
#include "top_level.h"

#include <wlr/types/wlr_keyboard.h>
#include <wlr/xwayland.h>
#include <wlr/types/wlr_seat.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/types/wlr_scene.h>
#include <xkbcommon/xkbcommon.h>

#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>

static bool IvyServer_HandleKeybinding(IvyServer *server, xkb_keysym_t sym, u32 keycode, u32 modifiers)
{
    if (keycode >= 10 && keycode <= 18)
    {
        int ws = keycode - 9;
        if (modifiers & WLR_MODIFIER_SHIFT) {
            if (server->seat->keyboard_state.focused_surface != NULL) {
                struct wlr_surface *focused_surface = server->seat->keyboard_state.focused_surface;
                IvyTopLevel *topLevel = NULL;

                struct wlr_xdg_toplevel *xdg_toplevel = wlr_xdg_toplevel_try_from_wlr_surface(focused_surface);
                if (xdg_toplevel != NULL) {
                    struct wlr_scene_tree *tree = xdg_toplevel->base->data;
                    topLevel = tree->node.data;
                } else {
                    struct wlr_xwayland_surface *xsurface = wlr_xwayland_surface_try_from_wlr_surface(focused_surface);
                    if (xsurface != NULL && xsurface->data != NULL) {
                        topLevel = xsurface->data;
                    }
                }

                if (topLevel != NULL) {
                    Ivy_TopLevel_MoveToWorkspace(topLevel, ws);
                }
            }
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
            if (wl_list_length(&server->topLevels) < 2) break;

            IvyTopLevel *next_topLevel = wl_container_of(server->topLevels.prev, next_topLevel, link);
            Ivy_TopLevel_Focus(next_topLevel);
            break;

        case XKB_KEY_t:
            if (fork() == 0) {
                execlp("thunar", "thunar", NULL);
                _exit(1);
            }
            break;

        case XKB_KEY_m:
            if (server->seat->keyboard_state.focused_surface != NULL)
            {
                struct wlr_surface *focused_surface = server->seat->keyboard_state.focused_surface;
                IvyTopLevel *topLevel = NULL;

                struct wlr_xdg_toplevel *xdg_toplevel = wlr_xdg_toplevel_try_from_wlr_surface(focused_surface);
                if (xdg_toplevel != NULL) {
                    struct wlr_scene_tree *tree = xdg_toplevel->base->data;
                    topLevel = tree->node.data;
                } else {
                    struct wlr_xwayland_surface *xsurface = wlr_xwayland_surface_try_from_wlr_surface(focused_surface);
                    if (xsurface != NULL && xsurface->data != NULL) {
                        topLevel = xsurface->data;
                    }
                }

                if (topLevel != NULL)
                {
                    Ivy_TopLevel_SetMaximize(topLevel, !topLevel->is_maximized);
                }
            }
            break;

        case XKB_KEY_f:
            if (server->seat->keyboard_state.focused_surface != NULL) {
                struct wlr_surface *focused_surface = server->seat->keyboard_state.focused_surface;
                IvyTopLevel *topLevel = NULL;

                struct wlr_xdg_toplevel *xdg_toplevel = wlr_xdg_toplevel_try_from_wlr_surface(focused_surface);
                if (xdg_toplevel != NULL) {
                    struct wlr_scene_tree *tree = xdg_toplevel->base->data;
                    topLevel = tree->node.data;
                } else {
                    struct wlr_xwayland_surface *xsurface = wlr_xwayland_surface_try_from_wlr_surface(focused_surface);
                    if (xsurface != NULL && xsurface->data != NULL) {
                        topLevel = xsurface->data;
                    }
                }

                if (topLevel != NULL) {
                    Ivy_TopLevel_SetFullscreen(topLevel, !topLevel->is_fullscreen);
                }
            }
            break;

        case XKB_KEY_q:
            if (server->seat->keyboard_state.focused_surface != NULL) {
                struct wlr_surface *focused_surface = server->seat->keyboard_state.focused_surface;
                
                struct wlr_xdg_toplevel *xdg_toplevel = wlr_xdg_toplevel_try_from_wlr_surface(focused_surface);
                if (xdg_toplevel != NULL) {
                    wlr_xdg_toplevel_send_close(xdg_toplevel);
                } else {
                    struct wlr_xwayland_surface *xsurface = wlr_xwayland_surface_try_from_wlr_surface(focused_surface);
                    if (xsurface != NULL) {
                        wlr_xwayland_surface_close(xsurface);
                    }
                }
            }
            break;

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

    u32 keycode = event->keycode + 8;

    const xkb_keysym_t *syms;
    int nsyms = xkb_state_key_get_syms(keyboard->wlr_keyboard->xkb_state, keycode, &syms);

    bool handled = false;
    u32 modifiers = wlr_keyboard_get_modifiers(keyboard->wlr_keyboard);

    if (modifiers & WLR_MODIFIER_ALT && event->state == WL_KEYBOARD_KEY_STATE_PRESSED)
    {
        for (int i = 0; i < nsyms; i++) {
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
