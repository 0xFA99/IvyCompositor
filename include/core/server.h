#ifndef IVY_SERVER_H
#define IVY_SERVER_H

#include "core/fwd.h"
#include "core/core.h"
#include "core/output.h"
#include "core/scene.h"
#include "shell/shell.h"
#include "input/seat.h"
#include "input/idle_inhibitor.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    IvySeat seat;
    struct wl_listener new_input;
} IvyServerInput;

struct IvyServer {
    IvyCore          core;
    IvyOutputManager output_manager;
    IvyScene         scene;
    IvyShell         shell;
    IvyServerInput   input;
    IvyIdle          idle;
};

void Ivy_Server_Init(IvyServer *server);

#ifdef __cplusplus
}
#endif

#endif
