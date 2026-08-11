#include "core/fwd.h"
#include "core/types.h"
#include "shell/shell.h"

void Ivy_Shell_Init(IvyShell *shell)
{
    IVY_ASSERT(shell != NULL, "[ERROR] IvyShell is NULL!");

    Ivy_XdgShell_Init(&shell->xdg_shell);

    Ivy_XdgTopLevelManager_Init(&shell->xdg_toplevel_manager);

    Ivy_XdgPopupManager_Init(&shell->xdg_popup_manager);
}