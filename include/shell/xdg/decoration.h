#ifndef IVY_SHELL_XDG_DECORATION_H
#define IVY_SHELL_XDG_DECORATION_H

#include "core/fwd.h"

#include <wayland-server-core.h>
#include <wlr/types/wlr_xdg_decoration_v1.h>

#ifdef __cplusplus
extern "C" {
#endif

struct IvyXdgDecoration
{
    struct wlr_xdg_toplevel_decoration_v1 *wlr_decoration;

    struct wl_listener destroy;
    struct wl_listener request_mode;
    struct wl_listener commit;
};

struct IvyXdgDecorationManager
{
    struct wlr_xdg_decoration_manager_v1 *wlr_decoration_manager;
    struct wl_listener new_toplevel_decoration;
};

void Ivy_XdgDecorationManager_Init(IvyXdgDecorationManager *decoration_manager);
void Ivy_XdgDecorationManager_Destroy(IvyXdgDecorationManager *decoration_manager);

#ifdef __cplusplus
}
#endif

#endif