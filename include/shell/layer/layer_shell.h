#ifndef IVY_SHELL_LAYER_SHELL_H
#define IVY_SHELL_LAYER_SHELL_H

#include "core/fwd.h"
#include "shell/layer/layer_surface.h"

#include <wlr/types/wlr_layer_shell_v1.h>

#ifdef __cplusplus
extern "C" {
#endif

struct IvyLayerShell {
    struct wlr_layer_shell_v1 *wlr_layer_shell;
    IvyLayerSurfaceManager surface_manager;
};

void Ivy_LayerShell_Init(IvyLayerShell *layer_shell);
void Ivy_LayerShell_Destroy(IvyLayerShell *layer_shell);

#ifdef __cplusplus
}
#endif

#endif