#ifndef IVY_SERVER_H
#define IVY_SERVER_H

#include "fwd.h"

#ifdef __cplusplus
extern "C" {
#endif

struct IvyServer {
    struct wl_display *wl_display;
    struct wlr_backend *backend;
    struct wlr_renderer *renderer;
    struct wlr_allocator *allocator;
};

void Ivy_Server_Init(IvyServer *server);
void Ivy_Server_Run(const IvyServer *restrict server, const char *restrict cmd);
void Ivy_Server_Destroy(const IvyServer *server);

#ifdef __cplusplus
}
#endif

#endif