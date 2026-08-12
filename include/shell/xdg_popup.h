#ifndef IVY_SHELL_POPUP_H
#define IVY_SHELL_POPUP_H

#include "core/fwd.h"

#include <wayland-server-core.h>
#include <wlr/types/wlr_xdg_shell.h>

#ifdef __cplusplus
extern "C" {
#endif

struct IvyXdgPopup {
    struct wlr_xdg_popup *xdg_popup;

    struct wl_list link;

    struct wl_listener commit;
    struct wl_listener destroy;
};

struct IvyXdgPopupManager {
    struct wl_list popups;
    struct wl_listener new_popup;
};

void Ivy_XdgPopupManager_Init(IvyXdgPopupManager *popup_manager);
void Ivy_XdgPopupManager_Destroy(IvyXdgPopupManager *popup_manager);

#ifdef __cplusplus
}
#endif

#endif