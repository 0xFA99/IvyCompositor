#include "core/fwd.h"
#include "core/types.h"
#include "shell/shell.h"
#include "shell/xdg/shell.h"
#include "shell/xdg/activation.h"
#include "shell/layer/layer_shell.h"

void Ivy_Shell_Init(IvyShell *shell)
{
    IVY_ASSERT(shell != NULL, "[ERROR] IvyShell is NULL!");

    Ivy_XdgShell_Init(&shell->xdg_shell);
    Ivy_XdgActivation_Init(&shell->xdg_activation);
    Ivy_LayerShell_Init(&shell->layer_shell);
    Ivy_Xwayland_Init(&shell->xwayland);
}

void Ivy_Shell_Destroy(IvyShell *shell)
{
    IVY_ASSERT(shell != NULL, "[ERROR] IvyShell is NULL!");

    Ivy_Xwayland_Destroy(&shell->xwayland);
    Ivy_XdgActivation_Destroy(&shell->xdg_activation);
    Ivy_LayerShell_Destroy(&shell->layer_shell);
    Ivy_XdgShell_Destroy(&shell->xdg_shell);
}