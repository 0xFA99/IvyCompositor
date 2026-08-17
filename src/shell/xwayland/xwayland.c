#include "core/fwd.h"
#include "core/types.h"
#include "shell/xwayland/xwayland.h"

#include <wayland-server-core.h>

static void IvyXwayland_HandleReady(struct wl_listener *listener, void *data);
static void IvyXwayland_HandleNewSurface(struct wl_listener *listener, void *data);

void Ivy_Xwayland_Init(IvyXwayland *xwayland)
{
    IVY_ASSERT(xwayland != NULL, "[ERROR] IvyXwayland is NULL!");
}

void Ivy_Xwayland_Destroy(IvyXwayland *xwayland)
{
    IVY_ASSERT(xwayland != NULL, "[ERROR] IvyXwayland is NULL!");
}

void Ivy_Xwayland_Focus(IvyXwayland *xwayland)
{
    IVY_ASSERT(xwayland != NULL, "[ERROR] IvyXwayland is NULL!");
}

static void IvyXwayland_HandleReady(struct wl_listener *listener, void *data)
{

}

static void IvyXwayland_HandleNewSurface(struct wl_listener *listener, void *data)
{

}
