#ifndef IVY_CORE_XDG_SHELL_H
#define IVY_CORE_XDG_SHELL_H

#include "core/fwd.h"
#include "shell/xdg_toplevel.h"
#include "shell/xdg_decoration.h"
#include "shell/xdg_popup.h"

#include <wlr/types/wlr_xdg_shell.h>

#ifdef __cplusplus
extern "C" {
#endif

struct IvyXdgShell {
    struct wlr_xdg_shell *wlr_xdg_shell;

    IvyXdgTopLevelManager xdg_toplevel_manager;
    IvyXdgDecorationManager xdg_decoration_manager;
    IvyXdgPopupManager xdg_popup_manager;
};

void Ivy_XdgShell_Init(IvyXdgShell *xdg_shell);
void Ivy_XdgShell_Destroy(IvyXdgShell *xdg_shell);

#ifdef __cplusplus
}
#endif

#endif