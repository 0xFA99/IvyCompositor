#ifndef IVY_POPUP_H
#define IVY_POPUP_H

#include <wayland-server-core.h>

#ifdef __cplusplus
extern "C" {
#endif

struct IvyPopup {
    struct wlr_xdg_popup *xdg_popup;
    struct wl_listener commit;
    struct wl_listener destroy;
};

void Ivy_Server_HandleNewXdgPopup(struct wl_listener *listener, void *data);

#ifdef __cplusplus
}
#endif

#endif