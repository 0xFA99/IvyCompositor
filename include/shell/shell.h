#ifndef IVY_SHELL_H
#define IVY_SHELL_H

#include "core/fwd.h"
#include "shell/xdg_shell.h"
#include "shell/xdg_toplevel.h"
#include "shell/xdg_popup.h"

#ifdef __cplusplus
extern "C" {
#endif

struct IvyShell {
    IvyXdgShell xdg_shell;
    IvyXdgTopLevelManager xdg_toplevel_manager;
    IvyXdgPopupManager xdg_popup_manager;
};

void Ivy_Shell_Init(IvyShell *shell);

#ifdef __cplusplus
}
#endif

#endif