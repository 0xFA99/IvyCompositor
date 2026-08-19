#ifndef IVY_SHELL_XWAYLAND_H
#define IVY_SHELL_XWAYLAND_H

#include "core/fwd.h"

#include <wayland-server-core.h>
#include <wlr/xwayland/xwayland.h>

#ifdef __cplusplus
extern "C" {
#endif

struct IvyXwayland
{
    struct wlr_xwayland *wlr_xwayland;
    struct wl_list surface;

    struct wl_listener ready;
    struct wl_listener new_surface;
};

void Ivy_Xwayland_Init(IvyXwayland *xwayland);
void Ivy_Xwayland_Destroy(IvyXwayland *xwayland);
void Ivy_Xwayland_Focus(const IvyXwaylandSurface *surface);

#ifdef __cplusplus
}
#endif

#endif