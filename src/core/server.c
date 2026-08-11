#include "core/types.h"
#include "core/server.h"
#include "core/scene.h"
#include "shell/shell.h"
#include "input/input.h"

#include <stdlib.h>

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

static void IvyServer_InitIdle(IvyServer *server)
{
    Ivy_Idle_Init(&server->idle);
}