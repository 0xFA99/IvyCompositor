#include "core/fwd.h"
#include "core/types.h"
#include "core/server.h"
#include "shell/xdg_shell.h"

#include <wayland-util.h>
#include <wlr/types/wlr_xdg_shell.h>

#define IVY_XDG_SHELL_VERSION           6
#define IVY_LAYER_SHELL_VERSION         4

void Ivy_XdgShell_Create(IvyXdgShell *xdg_shell)
{
    IVY_ASSERT(xdg_shell != NULL, "[ERROR] IvyXdgShell is NULL!");

    IvyServer *server = wl_container_of(xdg_shell, server, shell.xdg_shell);

    xdg_shell->wlr_xdg_shell = wlr_xdg_shell_create(server->core.wl_display, IVY_XDG_SHELL_VERSION);
    IVY_CHECK(xdg_shell->wlr_xdg_shell != NULL, "[WARNING] Failed to create wlr_xdg_shell!");
}
