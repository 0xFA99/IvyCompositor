#include "core/fwd.h"
#include "core/server.h"
#include "shell/xdg_toplevel.h"

#include <wayland-server-core.h>
#include <wlr/types/wlr_xdg_shell.h>

void Ivy_XdgTopLevelManager_Init(IvyXdgTopLevelManager *xdg_toplevel_manager)
{
    IvyXdgShell *xdg_shell = wl_container_of(xdg_toplevel_manager, xdg_shell, xdg_toplevel_manager);

    wl_list_init(&xdg_toplevel_manager->toplevels);
    wl_signal_add(&xdg_shell->wlr_xdg_shell->events.new_toplevel, &xdg_toplevel_manager->new_toplevel);
}
