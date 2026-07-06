#ifndef IVY_INPUT_H
#define IVY_INPUT_H

#include <wayland-server-core.h>

#ifdef __cplusplus
extern "C" {
#endif

void Ivy_Server_HandleNewInput(struct wl_listener *listener, void *data);

#ifdef __cplusplus
}
#endif

#endif