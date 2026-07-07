#include "types.h"
#include "server.h"
#include "cursor.h"

#include <wlr/types/wlr_xcursor_manager.h>
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/types/wlr_seat.h>

#include <stdlib.h>

static void IvyCursor_ProcessCursorMotion(IvyServer *server, u32 time);

static void IvyCursor_HandleMotion(struct wl_listener *listener, void *data);
static void IvyCursor_HandleMotionAbsolute(struct wl_listener *listener, void *data);
static void IvyCursor_HandleAxis(struct wl_listener *listener, void *data);
static void IvyCursor_HandleFrame(struct wl_listener *listener, void *data);

IvyCursor *Ivy_Cursor_Create(IvyServer *server)
{
    IvyCursor *cursor = calloc(1, sizeof(IvyCursor));
    IVY_CHECK(cursor != NULL, "[WARNING] Failed to allocate IvyCursor!");

    cursor->server = server;

    cursor->wlr_cursor = wlr_cursor_create();
    IVY_CHECK(cursor->wlr_cursor != NULL, "[WARNING] Failed to create wlr_cursor!");
    wlr_cursor_attach_output_layout(cursor->wlr_cursor, server->output_layout);

    cursor->cursor_mgr = wlr_xcursor_manager_create(NULL, 24);
    IVY_CHECK(cursor->cursor_mgr != NULL, "[WARNING] Failed to create xcursor manager!");
    wlr_xcursor_manager_load(cursor->cursor_mgr, 1);

    cursor->motion.notify = IvyCursor_HandleMotion;
    wl_signal_add(&cursor->wlr_cursor->events.motion, &cursor->motion);

    cursor->motion_absolute.notify = IvyCursor_HandleMotionAbsolute;
    wl_signal_add(&cursor->wlr_cursor->events.motion_absolute, &cursor->motion_absolute);

    cursor->axis.notify = IvyCursor_HandleAxis;
    wl_signal_add(&cursor->wlr_cursor->events.axis, &cursor->axis);

    cursor->frame.notify = IvyCursor_HandleFrame;
    wl_signal_add(&cursor->wlr_cursor->events.frame, &cursor->frame);

    return cursor;
}

void Ivy_Cursor_Destroy(IvyCursor *cursor)
{
    if (cursor == NULL) return;

    wlr_xcursor_manager_destroy(cursor->cursor_mgr);
    wlr_cursor_destroy(cursor->wlr_cursor);

    free(cursor);
}

void Ivy_Server_NewPointer(IvyServer *server, struct wlr_input_device *device)
{
    wlr_cursor_attach_input_device(server->cursor->wlr_cursor, device);
}

IvyTopLevel *Ivy_Desktop_TopLevelAt(IvyServer *server, double lx, double ly, struct wlr_surface **surface, double *sx, double *sy)
{
    struct wlr_scene_node *node = wlr_scene_node_at(&server->scene->tree.node, lx, ly, sx, sy);

    if (node == NULL || node->type != WLR_SCENE_NODE_BUFFER)
        return NULL;

    struct wlr_scene_buffer *scene_buffer = wlr_scene_buffer_from_node(node);
    struct wlr_scene_surface *scene_surface = wlr_scene_surface_try_from_buffer(scene_buffer);
    if (!scene_surface) return NULL;

    *surface = scene_surface->surface;

    struct wlr_scene_tree *tree = node->parent;
    while (tree != NULL && tree->node.data == NULL)
        tree = tree->node.parent;

    return tree ? tree->node.data : NULL;
}

static void IvyCursor_ProcessCursorMotion(IvyServer *server, u32 time)
{
    double sx, sy;
    struct wlr_surface *surface = NULL;

    IvyTopLevel *topLevel = Ivy_Desktop_TopLevelAt(server, server->cursor->wlr_cursor->x, server->cursor->wlr_cursor->y, &surface, &sx, &sy);

    if (!topLevel)
        wlr_cursor_set_xcursor(server->cursor->wlr_cursor, server->cursor->cursor_mgr, "default");

    if (surface) {
        wlr_seat_pointer_notify_enter(server->seat, surface, sx, sy);
        wlr_seat_pointer_notify_motion(server->seat, time, sx, sy);
    }
    else {
        wlr_seat_pointer_clear_focus(server->seat);
    }
}

static void IvyCursor_HandleMotion(struct wl_listener *listener, void *data)
{
    IvyCursor *cursor = wl_container_of(listener, cursor, motion);
    const struct wlr_pointer_motion_event *event = data;

    wlr_cursor_move(cursor->wlr_cursor, &event->pointer->base, event->delta_x, event->delta_y);
    IvyCursor_ProcessCursorMotion(cursor->server, event->time_msec);
}

static void IvyCursor_HandleMotionAbsolute(struct wl_listener *listener, void *data)
{
    IvyCursor *cursor = wl_container_of(listener, cursor, motion_absolute);
    const struct wlr_pointer_motion_absolute_event *event = data;

    wlr_cursor_warp_absolute(cursor->wlr_cursor, &event->pointer->base, event->x, event->y);
    IvyCursor_ProcessCursorMotion(cursor->server, event->time_msec);
}

static void IvyCursor_HandleAxis(struct wl_listener *listener, void *data)
{
    IvyCursor *cursor = wl_container_of(listener, cursor, axis);
    const struct wlr_pointer_axis_event *event = data;

    wlr_seat_pointer_notify_axis(
        cursor->server->seat, event->time_msec, event->orientation,
        event->delta, event->delta_discrete, event->source,
        event->relative_direction);
}

static void IvyCursor_HandleFrame(struct wl_listener *listener, void *data)
{
    IvyCursor *cursor = wl_container_of(listener, cursor, frame);
    (void)data;

    wlr_seat_pointer_notify_frame(cursor->server->seat);
}