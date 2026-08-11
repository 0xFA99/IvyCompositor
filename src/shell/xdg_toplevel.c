#include "core/fwd.h"
#include "core/server.h"
#include "shell/xdg_toplevel.h"

#include <wayland-server-core.h>
#include <wlr/types/wlr_xdg_shell.h>

#include <stdlib.h>
#include <wlr/types/wlr_cursor.h>

static void IvyXdgTopLevelManager_HandleNewTopLevel(struct wl_listener *listener, void *data);

static void IvyXdgTopLevel_HandleMap(struct wl_listener *listener, void *data);
static void IvyXdgTopLevel_HandleUnmap(struct wl_listener *listener, void *data);
static void IvyXdgTopLevel_HandleCommit(struct wl_listener *listener, void *data);
static void IvyXdgTopLevel_HandleDestroy(struct wl_listener *listener, void *data);

static void IvyXdgTopLevel_BeginInteractive(IvyXdgTopLevel *toplevel, IvyCursorMode mode, u32 edges);

static void IvyXdgTopLevel_HandleRequestMove(struct wl_listener *listener, void *data);
static void IvyXdgTopLevel_HandleRequestResize(struct wl_listener *listener, void *data);
static void IvyXdgTopLevel_HandleRequestMaximize(struct wl_listener *listener, void *data);
static void IvyXdgTopLevel_HandleRequestFullscreen(struct wl_listener *listener, void *data);

void Ivy_XdgTopLevelManager_Init(IvyXdgTopLevelManager *xdg_toplevel_manager)
{
    IvyXdgShell *xdg_shell = wl_container_of(xdg_toplevel_manager, xdg_shell, xdg_toplevel_manager);

    wl_list_init(&xdg_toplevel_manager->toplevels);

    xdg_toplevel_manager->new_toplevel.notify = IvyXdgTopLevelManager_HandleNewTopLevel;
    wl_signal_add(&xdg_shell->wlr_xdg_shell->events.new_toplevel, &xdg_toplevel_manager->new_toplevel);
}

IvyXdgTopLevel *Ivy_XdgTopLevel_SurfaceAt(IvyServer *server, double lx, double ly, struct wlr_surface **surface, double *sx, double *sy)
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

void Ivy_XdgTopLevel_Focus(IvyXdgTopLevel *toplevel)
{
    if (toplevel == NULL) return;
    // TODO: implement toplevel focus
}

static void IvyXdgTopLevelManager_HandleNewTopLevel(struct wl_listener *listener, void *data)
{
    IvyXdgTopLevelManager *manager = wl_container_of(listener, manager, new_toplevel);
    struct wlr_xdg_toplevel *wlr_toplevel = data;

    IvyXdgShell *xdg_shell = wl_container_of(manager, xdg_shell, xdg_toplevel_manager);
    IvyShell *shell = wl_container_of(xdg_shell, shell, xdg_shell);
    IvyServer *server = wl_container_of(shell, server, shell);

    IvyXdgTopLevel *toplevel = calloc(1, sizeof(IvyXdgTopLevel));
    IVY_CHECK(toplevel != NULL, "[WARNING] Failed to allocate IvyXdgTopLevel!");

    toplevel->server = server;
    toplevel->wlr_xdg_toplevel = wlr_toplevel;

    toplevel->map.notify = IvyXdgTopLevel_HandleMap;
    wl_signal_add(&wlr_toplevel->base->surface->events.map, &toplevel->map);

    toplevel->unmap.notify = IvyXdgTopLevel_HandleUnmap;
    wl_signal_add(&wlr_toplevel->base->surface->events.unmap, &toplevel->unmap);

    toplevel->commit.notify = IvyXdgTopLevel_HandleCommit;
    wl_signal_add(&wlr_toplevel->base->surface->events.commit, &toplevel->commit);

    toplevel->destroy.notify = IvyXdgTopLevel_HandleDestroy;
    wl_signal_add(&wlr_toplevel->events.destroy, &toplevel->destroy);

    wl_list_insert(&manager->toplevels, &toplevel->link);

    toplevel->request_move.notify = IvyXdgTopLevel_HandleRequestMove;
    wl_signal_add(&toplevel->wlr_xdg_toplevel->events.request_move, &toplevel->request_move);

    toplevel->request_resize.notify = IvyXdgTopLevel_HandleRequestResize;
    wl_signal_add(&toplevel->wlr_xdg_toplevel->events.request_resize, &toplevel->request_resize);

    toplevel->request_maximize.notify = IvyXdgTopLevel_HandleRequestMaximize;
    wl_signal_add(&toplevel->wlr_xdg_toplevel->events.request_maximize, &toplevel->request_maximize);

    toplevel->request_fullscreen.notify = IvyXdgTopLevel_HandleRequestFullscreen;
    wl_signal_add(&toplevel->wlr_xdg_toplevel->events.request_fullscreen, &toplevel->request_fullscreen);
}

static void IvyXdgTopLevel_HandleMap(struct wl_listener *listener, void *data)
{
    // TODO: implement handle map.
}

static void IvyXdgTopLevel_HandleUnmap(struct wl_listener *listener, void *data)
{
    // TODO: implement handle unmap.
}

static void IvyXdgTopLevel_HandleCommit(struct wl_listener *listener, void *data)
{
    // TODO: implement handle commit.
}

static void IvyXdgTopLevel_HandleDestroy(struct wl_listener *listener, void *data)
{
    IvyXdgTopLevel *toplevel = wl_container_of(listener, toplevel, destroy);
    (void)data;

    // wl_list_remove(&toplevel->map.link);
    // wl_list_remove(&toplevel->unmap.link);
    // wl_list_remove(&toplevel->commit.link);
    wl_list_remove(&toplevel->destroy.link);

    wl_list_remove(&toplevel->link);

    free(toplevel);
}

static void IvyXdgTopLevel_BeginInteractive(IvyXdgTopLevel *toplevel, IvyCursorMode mode, u32 edges)
{
    IvyCursor *cursor = &toplevel->server->input.cursor;

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

static void IvyXdgTopLevel_HandleRequestMove(struct wl_listener *listener, void *data)
{
    IvyXdgTopLevel *toplevel = wl_container_of(listener, toplevel, request_move);
    (void)data;

    IvyXdgTopLevel_BeginInteractive(toplevel, IVY_CURSOR_MOVE, 0);
}

static void IvyXdgTopLevel_HandleRequestResize(struct wl_listener *listener, void *data)
{
    IvyXdgTopLevel *toplevel = wl_container_of(listener, toplevel, request_resize);
    struct wlr_xdg_toplevel_resize_event *event = data;

    IvyXdgTopLevel_BeginInteractive(toplevel, IVY_CURSOR_RESIZE, event->edges);
}

static void IvyXdgTopLevel_HandleRequestMaximize(struct wl_listener *listener, void *data)
{
    IvyXdgTopLevel *toplevel = wl_container_of(listener, toplevel, request_maximize);
    (void)data;

    wlr_xdg_surface_schedule_configure(toplevel->wlr_xdg_toplevel->base);
}

static void IvyXdgTopLevel_HandleRequestFullscreen(struct wl_listener *listener, void *data)
{
    IvyXdgTopLevel *toplevel = wl_container_of(listener, toplevel, request_fullscreen);
    (void)data;

    if (toplevel->wlr_xdg_toplevel->base->initialized) {
        wlr_xdg_surface_schedule_configure(toplevel->wlr_xdg_toplevel->base);
    }
}
