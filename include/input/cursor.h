#ifndef IVY_INPUT_CURSOR_H
#define IVY_INPUT_CURSOR_H

#include "core/fwd.h"

#include <wayland-server-core.h>
#include <wlr/types/wlr_cursor.h>

typedef enum {
    IVY_CURSOR_PASSTHROUGH,
    IVY_CURSOR_RESIZE,
    IVY_CURSOR_MOVE
} IvyCursorMode;

#ifdef __cplusplus
extern "C" {
#endif

struct IvyCursor {
    struct wlr_cursor *wlr_cursor;

    struct wl_listener motion;
    struct wl_listener button;
    struct wl_listener axis;
    struct wl_listener frame;
    struct wl_listener grab_destroy;

    struct wl_listener request_cursor;
    struct wl_listener pointer_focus_change;

    IvyCursorMode mode;
};

void Ivy_Cursor_Init(IvyCursor *cursor);

#ifdef __cplusplus
}
#endif

#endif