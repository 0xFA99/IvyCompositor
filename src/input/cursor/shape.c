#include "core/fwd.h"
#include "core/types.h"
#include "core/server.h"
#include "input/input.h"
#include "input/cursor/cursor.h"
#include "input/cursor/shape.h"

#include <wayland-server-core.h>
#include <wlr/types/wlr_seat.h>
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_cursor_shape_v1.h>

#define IVY_CURSOR_SHAPE_VERSION 1

static void IvyCursorShape_HandleRequestSetShape(struct wl_listener *listener, void *data);

void Ivy_CursorShape_Init(IvyCursorShape *cursor_shape)
{
    IVY_ASSERT(cursor_shape != NULL, "[ERROR] IvyCursorShape is NULL!");

    IvyInput *input = wl_container_of(cursor_shape, input, cursor_shape);
    IvyServer *server = wl_container_of(input, server, input);

    cursor_shape->wlr_cursor_shape_manager = wlr_cursor_shape_manager_v1_create(server->core.wl_display, IVY_CURSOR_SHAPE_VERSION);
    IVY_CHECK(cursor_shape != NULL, "[WARNING] Failed to create wlr_cursor_shape_manager_v1!");

    cursor_shape->request_set_shape.notify = IvyCursorShape_HandleRequestSetShape;
    wl_signal_add(&cursor_shape->wlr_cursor_shape_manager->events.request_set_shape, &cursor_shape->request_set_shape);
}

void Ivy_CursorShape_Destroy(IvyCursorShape *cursor_shape)
{
    IVY_ASSERT(cursor_shape != NULL, "[ERROR] IvyCursorShape is NULL!");

    wl_list_remove(&cursor_shape->request_set_shape.link);
}

static void IvyCursorShape_HandleRequestSetShape(struct wl_listener *listener, void *data)
{
    IvyCursorShape *cursor_shape = wl_container_of(listener, cursor_shape, request_set_shape);
    const struct wlr_cursor_shape_manager_v1_request_set_shape_event *event = data;

    IvyInput *input = wl_container_of(cursor_shape, input, cursor_shape);
    IvyServer *server = wl_container_of(input, server, input);

    IvyCursor *cursor = &input->cursor;
    const struct wlr_seat_client *focused_client = input->seat.wlr_seat->pointer_state.focused_client;

    if (focused_client != event->seat_client)
        return;

    const char *shape_name = wlr_cursor_shape_v1_name(event->shape);

    wlr_cursor_set_xcursor(cursor->wlr_cursor, cursor->wlr_xcursor_manager, shape_name);
}
