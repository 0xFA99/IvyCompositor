#include "core/fwd.h"
#include "core/types.h"
#include "shell/shell.h"
#include "shell/xdg_shell.h"
#include "shell/layer_shell.h"

void Ivy_Shell_Init(IvyShell *shell)
{
    IVY_ASSERT(shell != NULL, "[ERROR] IvyShell is NULL!");

    Ivy_XdgShell_Init(&shell->xdg_shell);
    // Ivy_LayerShell_Init(&shell->layer_shell);
}

void Ivy_Shell_Destroy(IvyShell *shell)
{
    IVY_ASSERT(shell != NULL, "[ERROR] IvyShell is NULL!");

    Ivy_XdgShell_Destroy(&shell->xdg_shell);
}