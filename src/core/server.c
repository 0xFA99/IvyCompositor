#include "core/types.h"
#include "core/server.h"
#include "core/scene.h"
#include "shell/shell.h"
#include "input/input.h"

#include <wlr/backend.h>
#include <wlr/render/allocator.h>
#include <wlr/render/wlr_renderer.h>

#include <stdlib.h>
#include <unistd.h>

static void IvyServer_InitIdle(IvyServer *server);

void Ivy_Server_Init(IvyServer *server)
{
    IVY_ASSERT(server != NULL, "[ERROR] IvyServer is NULL!");

    Ivy_Core_Init(&server->core);

    Ivy_OutputManager_Init(&server->output_manager);
    Ivy_Scene_Init(&server->scene);
    Ivy_Shell_Init(&server->shell);
    Ivy_Input_Init(&server->input);
    IvyServer_InitIdle(server);
}

void Ivy_Server_Run(IvyServer *server, const char *startup_cmd)
{
    IVY_ASSERT(server != NULL, "[ERROR] IvyServer is NULL!");

    server->core.socket = wl_display_add_socket_auto(server->core.wl_display);
    IVY_CHECK(server->core.socket != NULL, "[ERROR] Failed to add Wayland socket!");

    if (!wlr_backend_start(server->core.wlr_backend)) {
        wlr_backend_destroy(server->core.wlr_backend);
        wl_display_destroy(server->core.wl_display);
        return;
    }

    setenv("WAYLAND_DISPLAY", server->core.socket, true);

    if (startup_cmd) {
        if (fork() == 0) {
            execl("/bin/sh", "/bin/sh", "-c", startup_cmd, (void *)NULL);
        }
    }

    wl_display_run(server->core.wl_display);
}

void Ivy_Server_Destroy(IvyServer *server)
{
    IVY_ASSERT(server != NULL, "[ERROR] IvyServer is NULL!");

    wl_display_destroy_clients(server->core.wl_display);

    Ivy_Idle_Destroy(&server->idle);
    Ivy_Shell_Destroy(&server->shell);
    Ivy_Input_Destroy(&server->input);
    Ivy_OutputManager_Destroy(&server->output_manager);
    Ivy_Scene_Destroy(&server->scene);

    // wlr_allocator_destroy(server->core.wlr_allocator);
    // wlr_renderer_destroy(server->core.wlr_renderer);
    wlr_backend_destroy(server->core.wlr_backend);
    wl_display_destroy(server->core.wl_display);
}

static void IvyServer_InitIdle(IvyServer *server)
{
    Ivy_Idle_Init(&server->idle);
}