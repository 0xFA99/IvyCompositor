#ifndef IVY_INPUT_CURSOR_SHAPE_H
#define IVY_INPUT_CURSOR_SHAPE_H

#include "core/fwd.h"

#include <wayland-server-core.h>
#include <wlr/types/wlr_cursor_shape_v1.h>

#ifdef __cplusplus
extern "C" {
#endif

struct IvyCursorShape {
    struct wlr_cursor_shape_manager_v1 *wlr_cursor_shape_manager;
    struct wl_listener request_set_shape;
};

void Ivy_CursorShape_Init(IvyCursorShape *cursor_shape);
void Ivy_CursorShape_Destroy(IvyCursorShape *cursor_shape);

#ifdef __cplusplus
}
#endif

#endif