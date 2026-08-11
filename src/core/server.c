#include "core/types.h"
#include "core/server.h"
#include "input/seat.h"

#include <stdlib.h>

static void IvyServer_InitInput(IvyServer *server);
static void IvyServer_InitIdle(IvyServer *server);

void Ivy_Server_Init(IvyServer *server)
{
    IVY_ASSERT(server != NULL, "[ERROR] IvyServer is NULL!");

    Ivy_Core_Init(&server->core);

    Ivy_OutputManager_Init(&server->output_manager);
    Ivy_Scene_Init(&server->scene);
    Ivy_Shell_Init(&server->shell);
    IvyServer_InitInput(server);
    IvyServer_InitIdle(server);
}

static void IvyServer_InitInput(IvyServer *server)
{
    Ivy_Seat_Init(&server->input.seat);

    // TODO: ....
    // Ivy_KeyboardManager_Create();
    // Ivy_Cursor_Create();
}

static void IvyServer_InitIdle(IvyServer *server)
{
    Ivy_Idle_Init(&server->idle);
}