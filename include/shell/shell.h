#ifndef IVY_SHELL_H
#define IVY_SHELL_H

#include "core/fwd.h"
#include "xdg/shell.h"
#include "layer/layer_shell.h"

#ifdef __cplusplus
extern "C" {
#endif

struct IvyShell {
    IvyXdgShell xdg_shell;
    IvyLayerShell layer_shell;
};

void Ivy_Shell_Init(IvyShell *shell);
void Ivy_Shell_Destroy(IvyShell *shell);

#ifdef __cplusplus
}
#endif

#endif