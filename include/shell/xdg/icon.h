#ifndef IVY_SHELL_XDG_TOPLEVEL_ICON_H
#define IVY_SHELL_XDG_TOPLEVEL_ICON_H

#include "core/fwd.h"

#include <wayland-server-core.h>
#include <wlr/types/wlr_xdg_toplevel_icon_v1.h>

#ifdef __cplusplus
extern "C" {
#endif

struct IvyXdgToplevelIconManager
{
    struct wlr_xdg_toplevel_icon_manager_v1 *wlr_icon_manager;
    struct wl_listener set_icon;
};

void Ivy_XdgToplevelIconManager_Init(IvyXdgToplevelIconManager *icon_manager);
void Ivy_XdgToplevelIconManager_Destroy(IvyXdgToplevelIconManager *icon_manager);

#ifdef __cplusplus
}
#endif

#endif