#include "core/fwd.h"
#include "core/server.h"
#include "shell/xdg/toplevel.h"

#include <wayland-server-core.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/types/wlr_cursor.h>

#include <stdlib.h>

static void IvyXdgToplevelManager_HandleNewTopLevel(struct wl_listener *listener, void *data);

static void IvyXdgToplevel_HandleMap(struct wl_listener *listener, void *data);
static void IvyXdgToplevel_HandleUnmap(struct wl_listener *listener, void *data);
static void IvyXdgToplevel_HandleCommit(struct wl_listener *listener, void *data);
static void IvyXdgToplevel_HandleDestroy(struct wl_listener *listener, void *data);

static void IvyXdgToplevel_BeginInteractive(IvyXdgToplevel *toplevel, IvyCursorMode mode, u32 edges);

static void IvyXdgToplevel_HandleRequestMove(struct wl_listener *listener, void *data);
static void IvyXdgToplevel_HandleRequestResize(struct wl_listener *listener, void *data);
static void IvyXdgToplevel_HandleRequestMaximize(struct wl_listener *listener, void *data);
static void IvyXdgToplevel_HandleRequestFullscreen(struct wl_listener *listener, void *data);

static int Ivy_DelayedTerminateCallback(void *data);

void Ivy_XdgToplevelManager_Init(IvyXdgToplevelManager *xdg_toplevel_manager)
{
    IvyXdgShell *xdg_shell = wl_container_of(xdg_toplevel_manager, xdg_shell, xdg_toplevel_manager);

    wl_list_init(&xdg_toplevel_manager->toplevels);

    xdg_toplevel_manager->new_toplevel.notify = IvyXdgToplevelManager_HandleNewTopLevel;
    wl_signal_add(&xdg_shell->wlr_xdg_shell->events.new_toplevel, &xdg_toplevel_manager->new_toplevel);
}

void Ivy_XdgToplevelManager_Destroy(IvyXdgToplevelManager *xdg_toplevel_manager)
{
    IVY_ASSERT(xdg_toplevel_manager != NULL, "[ERROR] IvyXdgToplevelManager is NULL!");

    wl_list_remove(&xdg_toplevel_manager->new_toplevel.link);
}

IvyXdgToplevel *Ivy_XdgToplevel_SurfaceAt(IvyServer *server, double lx, double ly, struct wlr_surface **surface, double *sx, double *sy)
{
    IVY_ASSERT(server != NULL, "[WARNING] IvyServer is NULL!");
    IVY_ASSERT(surface != NULL, "[WARNING] wlr_surface is NULL!");

    struct wlr_scene_node *node = wlr_scene_node_at(&server->scene.wlr_scene->tree.node, lx, ly, sx, sy);

    if (node == NULL || node->type != WLR_SCENE_NODE_BUFFER)
        return NULL;

    struct wlr_scene_buffer *scene_buffer = wlr_scene_buffer_from_node(node);
    struct wlr_scene_surface *scene_surface = wlr_scene_surface_try_from_buffer(scene_buffer);

    if (!scene_surface)
        return NULL;

    *surface = scene_surface->surface;
    struct wlr_scene_tree *tree = node->parent;

    while (tree != NULL && tree->node.data == NULL) {
        tree = tree->node.parent;
    }

    return tree->node.data;
}

void Ivy_XdgToplevel_Focus(IvyXdgToplevel *toplevel)
{
    if (toplevel == NULL) return;

    struct wlr_surface *prev_surface = toplevel->server->input.seat.wlr_seat->keyboard_state.focused_surface;
    struct wlr_surface *surface = toplevel->wlr_xdg_toplevel->base->surface;

    if (prev_surface == surface)
        return;

    if (prev_surface) {
        struct wlr_xdg_toplevel *prev_toplevel = wlr_xdg_toplevel_try_from_wlr_surface(prev_surface);
        if (prev_toplevel != NULL && prev_toplevel->base->initialized) {
            wlr_xdg_toplevel_set_activated(prev_toplevel, false);
        }
    }

    struct wlr_keyboard *wlr_keyboard = wlr_seat_get_keyboard(toplevel->server->input.seat.wlr_seat);

    wlr_scene_node_raise_to_top(&toplevel->wlr_scene_tree->node);
    wl_list_remove(&toplevel->link);
    wl_list_insert(&toplevel->server->shell.xdg_shell.xdg_toplevel_manager.toplevels, &toplevel->link);

    if (toplevel->wlr_xdg_toplevel->base->initialized) {
        wlr_xdg_toplevel_set_activated(toplevel->wlr_xdg_toplevel, true);
    }

    if (wlr_keyboard != NULL) {
        wlr_seat_keyboard_notify_enter(toplevel->server->input.seat.wlr_seat, surface,
            wlr_keyboard->keycodes, wlr_keyboard->num_keycodes, &wlr_keyboard->modifiers);
    }
}

static void IvyXdgToplevelManager_HandleNewTopLevel(struct wl_listener *listener, void *data)
{
    IvyXdgToplevelManager *manager = wl_container_of(listener, manager, new_toplevel);
    struct wlr_xdg_toplevel *wlr_toplevel = data;

    IvyXdgShell *xdg_shell = wl_container_of(manager, xdg_shell, xdg_toplevel_manager);
    IvyShell *shell = wl_container_of(xdg_shell, shell, xdg_shell);
    IvyServer *server = wl_container_of(shell, server, shell);

    IvyXdgToplevel *toplevel = calloc(1, sizeof(IvyXdgToplevel));
    IVY_CHECK(toplevel != NULL, "[WARNING] Failed to allocate IvyXdgToplevel!");

    toplevel->server = server;
    toplevel->wlr_xdg_toplevel = wlr_toplevel;

    toplevel->wlr_scene_tree = wlr_scene_xdg_surface_create(&server->scene.wlr_scene->tree, wlr_toplevel->base);
    toplevel->wlr_scene_tree->node.data = toplevel;
    wlr_toplevel->base->data = toplevel->wlr_scene_tree;

    toplevel->map.notify = IvyXdgToplevel_HandleMap;
    wl_signal_add(&wlr_toplevel->base->surface->events.map, &toplevel->map);

    toplevel->unmap.notify = IvyXdgToplevel_HandleUnmap;
    wl_signal_add(&wlr_toplevel->base->surface->events.unmap, &toplevel->unmap);

    toplevel->commit.notify = IvyXdgToplevel_HandleCommit;
    wl_signal_add(&wlr_toplevel->base->surface->events.commit, &toplevel->commit);

    toplevel->destroy.notify = IvyXdgToplevel_HandleDestroy;
    wl_signal_add(&wlr_toplevel->events.destroy, &toplevel->destroy);

    toplevel->request_move.notify = IvyXdgToplevel_HandleRequestMove;
    wl_signal_add(&toplevel->wlr_xdg_toplevel->events.request_move, &toplevel->request_move);

    toplevel->request_resize.notify = IvyXdgToplevel_HandleRequestResize;
    wl_signal_add(&toplevel->wlr_xdg_toplevel->events.request_resize, &toplevel->request_resize);

    toplevel->request_maximize.notify = IvyXdgToplevel_HandleRequestMaximize;
    wl_signal_add(&toplevel->wlr_xdg_toplevel->events.request_maximize, &toplevel->request_maximize);

    toplevel->request_fullscreen.notify = IvyXdgToplevel_HandleRequestFullscreen;
    wl_signal_add(&toplevel->wlr_xdg_toplevel->events.request_fullscreen, &toplevel->request_fullscreen);
}

static void IvyXdgToplevel_HandleMap(struct wl_listener *listener, void *data)
{
    IvyXdgToplevel *toplevel = wl_container_of(listener, toplevel, map);

    wl_list_insert(&toplevel->server->shell.xdg_shell.xdg_toplevel_manager.toplevels, &toplevel->link);

    Ivy_XdgToplevel_Focus(toplevel);
}

static void IvyXdgToplevel_HandleUnmap(struct wl_listener *listener, void *data)
{
    IvyXdgToplevel *toplevel = wl_container_of(listener, toplevel, unmap);
    (void)data;

    IvyCursor *cursor = &toplevel->server->input.cursor;

    if (toplevel == cursor->grab.toplevel) {
        cursor->grab.mode = IVY_CURSOR_PASSTHROUGH;
        cursor->grab.toplevel = NULL;
    }

    wl_list_remove(&toplevel->link);
}

static void IvyXdgToplevel_HandleCommit(struct wl_listener *listener, void *data)
{
    IvyXdgToplevel *toplevel = wl_container_of(listener, toplevel, commit);
    (void)data;

    if (toplevel->wlr_xdg_toplevel->base->initial_commit) {
        wlr_xdg_surface_schedule_configure(toplevel->wlr_xdg_toplevel->base);
    }
}

static void IvyXdgToplevel_HandleDestroy(struct wl_listener *listener, void *data)
{
    IvyXdgToplevel *toplevel = wl_container_of(listener, toplevel, destroy);
    const IvyServer *server = toplevel->server;
    (void)data;

    wl_list_remove(&toplevel->map.link);
    wl_list_remove(&toplevel->unmap.link);
    wl_list_remove(&toplevel->commit.link);
    wl_list_remove(&toplevel->destroy.link);

    wl_list_remove(&toplevel->request_move.link);
    wl_list_remove(&toplevel->request_resize.link);
    wl_list_remove(&toplevel->request_maximize.link);
    wl_list_remove(&toplevel->request_fullscreen.link);

    free(toplevel);

    const IvyXdgToplevelManager *manager = &server->shell.xdg_shell.xdg_toplevel_manager;
    if (server->is_terminating && wl_list_empty(&manager->toplevels)) {
        struct wl_event_loop *loop = wl_display_get_event_loop(server->core.wl_display);
        struct wl_event_source *source = wl_event_loop_add_timer(loop, Ivy_DelayedTerminateCallback, (void *)server);
        wl_event_source_timer_update(source, 50);
    }
}

static void IvyXdgToplevel_BeginInteractive(IvyXdgToplevel *toplevel, IvyCursorMode mode, u32 edges)
{
    IvyCursor *cursor = &toplevel->server->input.cursor;
    cursor->grab.toplevel = toplevel;
    cursor->grab.mode = mode;

    if (mode == IVY_CURSOR_MOVE)
    {
        cursor->grab.x = cursor->wlr_cursor->x - toplevel->wlr_scene_tree->node.x;
        cursor->grab.y = cursor->wlr_cursor->y - toplevel->wlr_scene_tree->node.y;
        return;
    }

    struct wlr_box *geo_box = &toplevel->wlr_xdg_toplevel->base->geometry;
    double border_x = (toplevel->wlr_scene_tree->node.x + geo_box->x) + ((edges & WLR_EDGE_RIGHT) ? geo_box->width : 0);
    double border_y = (toplevel->wlr_scene_tree->node.y + geo_box->y) + ((edges & WLR_EDGE_BOTTOM) ? geo_box->height : 0);

    cursor->grab.x = cursor->wlr_cursor->x - border_x;
    cursor->grab.y = cursor->wlr_cursor->y - border_y;

    cursor->grab.geo_box = *geo_box;
    cursor->grab.geo_box.x += toplevel->wlr_scene_tree->node.x;
    cursor->grab.geo_box.y += toplevel->wlr_scene_tree->node.y;

    cursor->grab.resize_edges = edges;
}

static void IvyXdgToplevel_HandleRequestMove(struct wl_listener *listener, void *data)
{
    IvyXdgToplevel *toplevel = wl_container_of(listener, toplevel, request_move);
    (void)data;

    IvyXdgToplevel_BeginInteractive(toplevel, IVY_CURSOR_MOVE, 0);
}

static void IvyXdgToplevel_HandleRequestResize(struct wl_listener *listener, void *data)
{
    IvyXdgToplevel *toplevel = wl_container_of(listener, toplevel, request_resize);
    struct wlr_xdg_toplevel_resize_event *event = data;

    IvyXdgToplevel_BeginInteractive(toplevel, IVY_CURSOR_RESIZE, event->edges);
}

static void IvyXdgToplevel_HandleRequestMaximize(struct wl_listener *listener, void *data)
{
    IvyXdgToplevel *toplevel = wl_container_of(listener, toplevel, request_maximize);
    (void)data;

    if (toplevel->wlr_xdg_toplevel->base->initialized) {
        wlr_xdg_surface_schedule_configure(toplevel->wlr_xdg_toplevel->base);
    }
}

static void IvyXdgToplevel_HandleRequestFullscreen(struct wl_listener *listener, void *data)
{
    IvyXdgToplevel *toplevel = wl_container_of(listener, toplevel, request_fullscreen);
    (void)data;

    if (toplevel->wlr_xdg_toplevel->base->initialized) {
        wlr_xdg_surface_schedule_configure(toplevel->wlr_xdg_toplevel->base);
    }
}

static int Ivy_DelayedTerminateCallback(void *data)
{
    IvyServer *server = data;
    wl_display_terminate(server->core.wl_display);
    return 0;
}
