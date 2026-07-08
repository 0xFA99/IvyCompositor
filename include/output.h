#ifndef IVY_OUTPUT_H
#define IVY_OUTPUT_H

#include "fwd.h"

#include <wayland-server-core.h>
#include <wayland-util.h>

#ifdef __cplusplus
extern "C" {
#endif

struct IvyOutput {
    IvyServer           *server;
    struct wlr_output   *wlr_output;

    struct wl_list      link;
    struct wl_listener  frame;
    struct wl_listener  request_state;
    struct wl_listener  destroy;
};

void Ivy_Server_HandleNewOutput(struct wl_listener *listener, void *data);

#ifdef __cplusplus
}
#endif

#endif