#ifndef IVY_SHELL_XDG_ACTIVATION_H
#define IVY_SHELL_XDG_ACTIVATION_H

#ifdef __cplusplus
extern "C" {
#endif

#include "core/fwd.h"

#include <wayland-server-core.h>
#include <wlr/types/wlr_xdg_activation_v1.h>

struct IvyXdgActivation
{
    struct wlr_xdg_activation_v1 *wlr_activation;
    struct wl_listener request_activate;
};

void Ivy_XdgActivation_Init(IvyXdgActivation *activation);
void Ivy_XdgActivation_Destroy(IvyXdgActivation *activation);

#ifdef __cplusplus
}
#endif

#endif