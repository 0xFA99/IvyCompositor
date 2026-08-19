#ifndef IVY_SHELL_H
#define IVY_SHELL_H

#include "core/fwd.h"
#include "shell/xdg/shell.h"
#include "shell/xdg/activation.h"
#include "shell/xwayland/xwayland.h"
#include "layer/layer_shell.h"

#ifdef __cplusplus
extern "C" {
#endif

struct IvyShell {
    IvyXdgShell xdg_shell;
    IvyXdgActivation xdg_activation;
    IvyLayerShell layer_shell;
    IvyXwayland xwayland;
};

void Ivy_Shell_Init(IvyShell *shell);
void Ivy_Shell_Destroy(IvyShell *shell);

#ifdef __cplusplus
}
#endif

#endif