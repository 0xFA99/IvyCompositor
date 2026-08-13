#include "core/fwd.h"
#include "core/types.h"
#include "core/server.h"
#include "shell/xdg_shell.h"

#include <wayland-util.h>
#include <wlr/types/wlr_xdg_shell.h>

#define IVY_XDG_SHELL_VERSION           6
#define IVY_LAYER_SHELL_VERSION         4

void Ivy_XdgShell_Init(IvyXdgShell *xdg_shell)
{
    IVY_ASSERT(xdg_shell != NULL, "[ERROR] IvyXdgShell is NULL!");

    IvyServer *server = wl_container_of(xdg_shell, server, shell.xdg_shell);

    xdg_shell->wlr_xdg_shell = wlr_xdg_shell_create(server->core.wl_display, IVY_XDG_SHELL_VERSION);
    IVY_CHECK(xdg_shell->wlr_xdg_shell != NULL, "[WARNING] Failed to create wlr_xdg_shell!");

    Ivy_XdgTopLevelManager_Init(&xdg_shell->xdg_toplevel_manager);
    Ivy_XdgDecorationManager_Init(&xdg_shell->xdg_decoration_manager);
    Ivy_XdgPopupManager_Init(&xdg_shell->xdg_popup_manager);
}

void Ivy_XdgShell_Destroy(IvyXdgShell *xdg_shell)
{
    IVY_ASSERT(xdg_shell != NULL, "[ERROR] IvyXdgShell is NULL!");

    Ivy_XdgTopLevelManager_Destroy(&xdg_shell->xdg_toplevel_manager);
    Ivy_XdgDecorationManager_Destroy(&xdg_shell->xdg_decoration_manager);
    Ivy_XdgPopupManager_Destroy(&xdg_shell->xdg_popup_manager);
}

