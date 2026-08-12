#include "core/fwd.h"
#include "core/types.h"
#include "core/server.h"
#include "shell/layer_shell.h"
#include "shell/layer_surface.h"

#include <wayland-server-core.h>

#define IVY_LAYER_SHELL_VERSION 4

void Ivy_LayerShell_Init(IvyLayerShell *layer_shell)
{
    IVY_ASSERT(layer_shell != NULL, "[ERROR] IvyLayerShell is NULL!");

    IvyServer *server = wl_container_of(layer_shell, server, shell.layer_shell);

    layer_shell->wlr_layer_shell = wlr_layer_shell_v1_create(server->core.wl_display, IVY_LAYER_SHELL_VERSION);
    IVY_CHECK(layer_shell != NULL, "[WARNING] Failed to create wlr_layer_shell!");

    // IvyLayerSurfaceManager *manager = &layer_shell->surface_manager;
    // wl_list_init(&manager->surfaces);

    // manager->new_surface.notify = Ivy_LayerSurfaceManager_HandleNewSurface;
    // wl_signal_add(&layer_shell->wlr_layer_shell->events.new_surface, &manager->new_surface);
}