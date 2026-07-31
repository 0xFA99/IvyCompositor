#ifndef IVY_SHELL_LAYER_SHELL_H
#define IVY_SHELL_LAYER_SHELL_H

#include "core/fwd.h"
#include "shell/xdg_toplevel.h"
#include "shell/xdg_popup.h"

#include <wlr/types/wlr_layer_shell_v1.h>

#ifdef __cplusplus
extern "C" {
#endif

struct IvyLayerShell {
    struct wlr_layer_shell_v1 *wlr_layer_shell;

    IvyXdgTopLevelManager xdg_top_level_manager;
    IvyXdgPopupManager xdg_popup_manager;
};

void Ivy_LayerShell_Init(IvyLayerShell *layer_shell);

#ifdef __cplusplus
}
#endif

#endif