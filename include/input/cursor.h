#ifndef IVY_INPUT_CURSOR_H
#define IVY_INPUT_CURSOR_H

#include "core/fwd.h"
#include "core/types.h"

#include <wayland-server-core.h>
#include <wlr/util/box.h>

#define IVY_CURSOR_DEFAULT_STYLE "default"
#define IVY_CURSOR_DEFAULT_SIZE 24

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    IVY_CURSOR_PASSTHROUGH,
    IVY_CURSOR_RESIZE,
    IVY_CURSOR_MOVE
} IvyCursorMode;

typedef struct {
    IvyCursorMode   mode;
    IvyXdgTopLevel  *toplevel;
    double          x, y;
    struct wlr_box  geo_box;
    u32             resize_edges;
} IvyCursorGrab;

struct IvyCursor {
    struct wlr_cursor *wlr_cursor;
    struct wlr_xcursor_manager *wlr_xcursor_manager;

    IvyInput *input;

    struct wlr_surface *focused_surface;

    struct wl_listener motion;
    struct wl_listener button;
    struct wl_listener axis;
    struct wl_listener frame;
    struct wl_listener grab_destroy;

    struct wl_listener request_cursor;
    struct wl_listener pointer_focus_change;

    IvyCursorGrab grab;
};

void Ivy_Cursor_Init(IvyCursor *cursor, IvyInput *input);
void Ivy_Cursor_UpdateScale(IvyCursor *cursor, float scale);

#ifdef __cplusplus
}
#endif

#endif