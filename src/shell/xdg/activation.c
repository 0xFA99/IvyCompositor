#include "core/fwd.h"
#include "core/types.h"
#include "core/server.h"
#include "shell/shell.h"
#include "shell/xdg/toplevel.h"
#include "shell/xdg/activation.h"

#include <wayland-server-core.h>
#include <wlr/types/wlr_seat.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/types/wlr_xdg_activation_v1.h>

#define IVY_XDG_ACTIVATION_TOKEN_TIMEOUT_MS 10000

static void IvyXdgActivation_HandleRequestActivation(struct wl_listener *listener, void *data);

void Ivy_XdgActivation_Init(IvyXdgActivation *activation)
{
    IVY_ASSERT(activation != NULL, "[ERROR] IvyXdgActivation is NULL!");

    IvyShell *shell = wl_container_of(activation, shell, xdg_activation);
    IvyServer *server = wl_container_of(shell, server, shell);

    activation->wlr_activation = wlr_xdg_activation_v1_create(server->core.wl_display);
    IVY_CHECK(activation->wlr_activation != NULL, "[WARNING] Failed to create wlr_xdg_activation_v1!");

    activation->wlr_activation->token_timeout_msec = IVY_XDG_ACTIVATION_TOKEN_TIMEOUT_MS;

    activation->request_activate.notify = IvyXdgActivation_HandleRequestActivation;
    wl_signal_add(&activation->wlr_activation->events.request_activate, &activation->request_activate);
}
void Ivy_XdgActivation_Destroy(IvyXdgActivation *activation)
{
    IVY_ASSERT(activation != NULL, "[ERROR] IvyXdgActivation is NULL!");

    wl_list_remove(&activation->request_activate.link);
}

static void IvyXdgActivation_HandleRequestActivation(struct wl_listener *listener, void *data)
{
    IvyXdgActivation *activation = wl_container_of(listener, activation, request_activate);
    const struct wlr_xdg_activation_v1_request_activate_event *event = data;

    if (event->surface == NULL)
        return;

    IvyShell *shell = wl_container_of(activation, shell, xdg_activation);
    IvyServer *server = wl_container_of(shell, server, shell);

    struct wlr_xdg_toplevel *wlr_toplevel = wlr_xdg_toplevel_try_from_wlr_surface(event->surface);

    if (wlr_toplevel == NULL)
        return;

    struct wlr_scene_tree *scene_tree = wlr_toplevel->base->data;
    if (scene_tree == NULL)
        return;

    IvyXdgToplevel *toplevel = scene_tree->node.data;
    if (toplevel == NULL)
        return;

    const struct wlr_xdg_activation_token_v1 *token = event->token;
    bool should_focus = false;

    if (token->seat != NULL)
    {
        const struct wlr_surface *focused_surface = token->seat->keyboard_state.focused_surface;

        if (token->surface != NULL && focused_surface == token->surface) {
            should_focus = true;
        } else if (token->surface == NULL) {
            should_focus = true;
        }
    } else if (token->surface != NULL) {
        should_focus = true;
    }

    if (should_focus) {
        Ivy_XdgToplevel_Focus(toplevel);
    } else {
        // TODO: implement visual hint
    }
}
