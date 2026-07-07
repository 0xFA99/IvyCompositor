#ifndef IVY_CURSOR_H
#define IVY_CURSOR_H

#include "fwd.h"

#include <wayland-server-core.h>
#include <wlr/types/wlr_input_device.h>
#include <wlr/types/wlr_compositor.h>
#include <wlr/util/box.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    IVY_CURSOR_PASSTHROUGH,
    IVY_CURSOR_MOVE,
    IVY_CURSOR_RESIZE
} IvyCursorMode;

struct IvyCursor {
    IvyServer *server;
    struct wlr_cursor *wlr_cursor;
    struct wlr_xcursor_manager *cursor_mgr;

    struct wl_listener motion;
    struct wl_listener motion_absolute;
    struct wl_listener axis;
    struct wl_listener frame;
    struct wl_listener button;

    IvyCursorMode mode;
    IvyTopLevel *grabbed_topLevel;
    double grab_x, grab_y;
    struct wlr_box grab_geoBox;
    u32 resize_edges;
};

IvyCursor *Ivy_Cursor_Create(IvyServer *server);
void Ivy_Cursor_Destroy(IvyCursor *cursor);

void Ivy_Server_NewPointer(IvyServer *server, struct wlr_input_device *device);

IvyTopLevel *Ivy_Desktop_TopLevelAt(IvyServer *server, double lx, double ly, struct wlr_surface **surface, double *sx, double *sy);

void Ivy_Cursor_BeginInteraction(IvyCursor *cursor, IvyTopLevel *topLevel, IvyCursorMode mode, u32 edges);

#ifdef __cplusplus
}
#endif

#endif