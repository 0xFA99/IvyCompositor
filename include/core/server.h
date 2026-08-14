#ifndef IVY_SERVER_H
#define IVY_SERVER_H

#include "core/fwd.h"
#include "core/core.h"
#include "core/output.h"
#include "core/scene.h"
#include "input/input.h"
#include "shell/shell.h"
#include "input/idle_inhibitor.h"

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

struct IvyServer {
    IvyCore          core;
    IvyOutputManager output_manager;
    IvyScene         scene;
    IvyShell         shell;
    IvyInput         input;
    IvyIdle          idle;

    bool             is_terminating;
};

void Ivy_Server_Init(IvyServer *server);
void Ivy_Server_Run(IvyServer *server, const char *startup_cmd);
void Ivy_Server_Destroy(IvyServer *server);

#ifdef __cplusplus
}
#endif

#endif
