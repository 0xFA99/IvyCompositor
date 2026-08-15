#include "core/fwd.h"
#include "core/types.h"
#include "core/server.h"
#include "core/core.h"

#include <wayland-server-core.h>

#include <wlr/backend.h>
#include <wlr/render/allocator.h>
#include <wlr/render/wlr_renderer.h>
#include <wlr/types/wlr_compositor.h>
#include <wlr/types/wlr_data_device.h>
#include <wlr/types/wlr_subcompositor.h>
#include <wlr/types/wlr_primary_selection_v1.h>

#include <stdbool.h>
#include <stdlib.h>

#define IVY_COMPOSITOR_PROTOCOL_VERSION 5

void Ivy_Core_Init(IvyCore *core)
{
    IVY_ASSERT(core != NULL, "[ERROR] IvyCore is NULL!");

    // display
    core->wl_display = wl_display_create();
    IVY_CHECK(core->wl_display != NULL, "[WARNING] Failed to create wl_display!");

    // backend
    struct wl_event_loop *wl_event_loop = wl_display_get_event_loop(core->wl_display);
    core->wlr_backend = wlr_backend_autocreate(wl_event_loop, NULL);
    IVY_CHECK(core->wlr_backend != NULL, "[WARNING] Failed to create wlr_backend!");

    // renderer
    core->wlr_renderer = wlr_renderer_autocreate(core->wlr_backend);
    IVY_CHECK(core->wlr_renderer != NULL, "[WARNING] Failed to create wlr_renderer!");

    wlr_renderer_init_wl_display(core->wlr_renderer, core->wl_display);

    // allocator
    core->wlr_allocator = wlr_allocator_autocreate(core->wlr_backend, core->wlr_renderer);
    IVY_CHECK(core->wlr_allocator != NULL, "[WARNING] Failed to create wlr_allocator!");

    // compositor
    core->wlr_compositor = wlr_compositor_create(
        core->wl_display,
        IVY_COMPOSITOR_PROTOCOL_VERSION,
        core->wlr_renderer
    );
    IVY_CHECK(core->wlr_compositor != NULL, "[WARNING] Failed to create wlr_compositor!");

    // (popup, tooltips, sub-surfaces)
    struct wlr_subcompositor *subcompositor = wlr_subcompositor_create(core->wl_display);
    IVY_CHECK(subcompositor != NULL, "[WARNING] Failed to create wlr_subcompositor!");

    // (clipboard, drag-and-drop)
    struct wlr_data_device_manager *data_device_manager = wlr_data_device_manager_create(core->wl_display);
    IVY_CHECK(data_device_manager != NULL, "[WARNING] Failed to create wlr_data_device_manager!");

    struct wlr_primary_selection_v1_device_manager *primary_selection_manager = wlr_primary_selection_v1_device_manager_create(core->wl_display);
    IVY_CHECK(primary_selection_manager != NULL, "[WARNING] Failed to create wlr_primary_selection_v1_device_manager!");
}

void Ivy_Core_Start(IvyCore *core)
{
    IVY_ASSERT(core != NULL, "[ERROR] IvyCore is NULL!");

    core->socket = wl_display_add_socket_auto(core->wl_display);
    IVY_CHECK(core->socket != NULL, "[WARNING] Failed to add socket!");

    bool status = wlr_backend_start(core->wlr_backend);
    IVY_CHECK(status != false, "[WARNING] Failed to start backend!");
}

void Ivy_Core_AddSocket(IvyCore *core)
{
    IVY_ASSERT(core != NULL, "[ERROR] IvyCore is NULL!");

    core->socket = wl_display_add_socket_auto(core->wl_display);
    IVY_CHECK(core->socket != NULL, "[WARNING] Failed to add socket!");
}

void Ivy_Core_StartSocket(IvyCore *core)
{
    IVY_ASSERT(core != NULL, "[ERROR] IvyCore is NULL!");

    if (!wlr_backend_start(core->wlr_backend)) {
        wlr_backend_destroy(core->wlr_backend);
        wl_display_destroy(core->wl_display);
        return;
    }

    bool status = wlr_backend_start(core->wlr_backend);
    IVY_CHECK(status != false, "[WARNING] Failed to start backend!");

    setenv("WAYLAND_DISPLAY", core->socket, true);
    wl_display_run(core->wl_display);
}

void Ivy_Core_Destroy(const IvyCore *core)
{
    IVY_ASSERT(core != NULL, "[ERROR] IvyCore is NULL!");

    wl_display_destroy_clients(core->wl_display);
    wl_display_destroy(core->wl_display);
}