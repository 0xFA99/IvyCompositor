#include "core/fwd.h"
#include "core/types.h"
#include "core/server.h"
#include "shell/shell.h"
#include "shell/xwayland/xwayland.h"
#include "shell/xwayland/surface.h"

#include <wayland-server-core.h>

static void IvyXwayland_HandleReady(struct wl_listener *listener, void *data);
static void IvyXwayland_HandleNewSurface(struct wl_listener *listener, void *data);

void Ivy_Xwayland_Init(IvyXwayland *xwayland)
{
    IVY_ASSERT(xwayland != NULL, "[ERROR] IvyXwayland is NULL!");

    IvyShell *shell = wl_container_of(xwayland, shell, xwayland);
    IvyServer *server = wl_container_of(shell, server, shell);

    xwayland->wlr_xwayland = wlr_xwayland_create(server->core.wl_display, server->core.wlr_compositor, true);
    IVY_CHECK(xwayland->wlr_xwayland != NULL, "[WARNING] Failed to create wlr_xwayland!");

    xwayland->ready.notify = IvyXwayland_HandleReady;
    wl_signal_add(&xwayland->wlr_xwayland->events.ready, &xwayland->ready);

    xwayland->new_surface.notify = IvyXwayland_HandleNewSurface;
    wl_signal_add(&xwayland->wlr_xwayland->events.new_surface, &xwayland->new_surface);
}

void Ivy_Xwayland_Destroy(IvyXwayland *xwayland)
{
    IVY_ASSERT(xwayland != NULL, "[ERROR] IvyXwayland is NULL!");

    wl_list_remove(&xwayland->ready.link);
    wl_list_remove(&xwayland->new_surface.link);

    wlr_xwayland_destroy(xwayland->wlr_xwayland);
}

void Ivy_Xwayland_Focus(const IvyXwaylandSurface *surface)
{
    IVY_ASSERT(surface != NULL, "[ERROR] IvyXwayland is NULL!");

    struct wlr_xwayland_surface *xsurface = surface->wlr_xwayland_surface;
    const IvyServer *server = surface->server;
    struct wlr_seat *seat = server->input.seat.wlr_seat;

    const enum wlr_xwayland_icccm_input_model model = wlr_xwayland_surface_icccm_input_model(xsurface);

    if (model == WLR_ICCCM_INPUT_MODEL_NONE)
        return;

    wlr_xwayland_surface_activate(xsurface, true);

    if (model == WLR_ICCCM_INPUT_MODEL_GLOBAL) {
        wlr_xwayland_surface_offer_focus(xsurface);
        return;
    }

    const struct wlr_keyboard *keyboard = wlr_seat_get_keyboard(seat);
    if (keyboard != NULL) {
        wlr_seat_keyboard_notify_enter(seat, xsurface->surface, keyboard->keycodes, keyboard->num_keycodes, &keyboard->modifiers);
    }
}

static void IvyXwayland_HandleReady(struct wl_listener *listener, void *data)
{
    IvyXwayland *xwayland = wl_container_of(listener, xwayland, ready);
    (void)data;

    IvyShell *shell = wl_container_of(xwayland, shell, xwayland);
    IvyServer *server = wl_container_of(shell, server, shell);

    setenv("DISPLAY", xwayland->wlr_xwayland->display_name, true);
    wlr_xwayland_set_seat(xwayland->wlr_xwayland, server->input.seat.wlr_seat);
}

static void IvyXwayland_HandleNewSurface(struct wl_listener *listener, void *data)
{
    IvyXwayland *xwayland = wl_container_of(listener, xwayland, new_surface);
    struct wlr_xwayland_surface *wlr_xwayland_surface = data;

    IvyShell *shell = wl_container_of(xwayland, shell, xwayland);
    IvyServer *server = wl_container_of(shell, server, shell);

    Ivy_Xwayland_Create(server, wlr_xwayland_surface);
}
