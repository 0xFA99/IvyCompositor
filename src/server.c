#include "types.h"
#include "server.h"

#include <wlr/backend.h>
#include <wlr/render/allocator.h>
#include <wlr/render/wlr_renderer.h>

#include <stdlib.h>

void Ivy_Server_Init(IvyServer *server)
{
    IVY_ASSERT(server != NULL, "[ERROR] IvyServer is NULL!");

    server->wl_display = wl_display_create();
    IVY_CHECK(server->wl_display != NULL, "[WARNING] Failed to create wl_display!");

    server->backend = wlr_backend_autocreate(wl_display_get_event_loop(server->wl_display), NULL);
    IVY_CHECK(server->backend != NULL, "[WARNING] Failed to create wlr_backend!");

    server->renderer = wlr_renderer_autocreate(server->backend);
    IVY_CHECK(server->renderer != NULL, "[WARNING] Failed to create wlr_renderer!");

    wlr_renderer_init_wl_display(server->renderer, server->wl_display);

    server->allocator = wlr_allocator_autocreate(server->backend, server->renderer);
    IVY_CHECK(server->allocator != NULL, "[WARNING] Failed to create wlr_allocator!");
}

void Ivy_Server_Run(const IvyServer *restrict server, const char *restrict cmd)
{
    IVY_ASSERT(server != NULL, "[ERROR] IvyServer is NULL!");
    (void)cmd;

    const char *socket = wl_display_add_socket_auto(server->wl_display);
    IVY_CHECK(socket != NULL, "[WARNING] Failed to create Wayland socket!");

    setenv("WAYLAND_DISPLAY", socket, true);

    wl_display_run(server->wl_display);
}

void Ivy_Server_Destroy(const IvyServer *server)
{
    IVY_ASSERT(server != NULL, "[ERROR] IvyServer is NULL!");

    wlr_allocator_destroy(server->allocator);
    wlr_renderer_destroy(server->renderer);
    wlr_backend_destroy(server->backend);

    wl_display_destroy(server->wl_display);
}
