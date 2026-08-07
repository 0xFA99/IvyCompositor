#include "core/types.h"
#include "core/server.h"
#include "shell/xdg_shell.h"
#include "shell/layer_shell.h"
#include "input/seat.h"

#include <wayland-server-core.h>
#include <wlr/backend.h>
#include <wlr/render/allocator.h>
#include <wlr/render/wlr_renderer.h>
#include <wlr/types/wlr_compositor.h>
#include <wlr/types/wlr_subcompositor.h>
#include <wlr/types/wlr_output_layout.h>
#include <wlr/types/wlr_data_device.h>
#include <wlr/types/wlr_scene.h>

#define IVY_COMPOSITOR_PROTOCOL_VERSION 5

static void IvyServer_InitCore(IvyServer *server);
static void IvyServer_InitOutput(IvyServer *server);
static void IvyServer_InitScene(IvyServer *server);
static void IvyServer_InitShells(IvyServer *server);
static void IvyServer_InitInput(IvyServer *server);
static void IvyServer_InitIdle(IvyServer *server);

void Ivy_Server_Init(IvyServer *server)
{
    IVY_ASSERT(server != NULL, "[ERROR] IvyServer is NULL!");

    IvyServer_InitCore(server);
    IvyServer_InitOutput(server);
    IvyServer_InitScene(server);
    IvyServer_InitShells(server);
    IvyServer_InitInput(server);
    IvyServer_InitIdle(server);
}

static void IvyServer_InitCore(IvyServer *server)
{
    IvyServerCore *core = &server->core;

    // Display
    core->wl_display = wl_display_create();
    IVY_CHECK(core->wl_display != NULL, "[WARNING] Failed to create wl_display!");

    // Backend
    struct wl_event_loop *wl_event_loop = wl_display_get_event_loop(core->wl_display);
    core->wlr_backend = wlr_backend_autocreate(wl_event_loop, NULL);
    IVY_CHECK(core->wlr_backend != NULL, "[WARNING] Failed to create wlr_backend!");

    // Renderer
    core->wlr_renderer = wlr_renderer_autocreate(core->wlr_backend);
    IVY_CHECK(core->wlr_renderer != NULL, "[WARNING] Failed to create wlr_renderer!");

    wlr_renderer_init_wl_display(core->wlr_renderer, core->wl_display);

    // Allocator
    core->wlr_allocator = wlr_allocator_autocreate(core->wlr_backend, core->wlr_renderer);
    IVY_CHECK(core->wlr_allocator != NULL, "[WARNING] Failed to create wlr_allocator!");

    core->wlr_compositor = wlr_compositor_create(server->core.wl_display, IVY_COMPOSITOR_PROTOCOL_VERSION, server->core.wlr_renderer);
    IVY_CHECK(core->wlr_compositor != NULL, "[WARNING] Failed to create wlr_compositor!");

    // Popup, Tooltips, Window-child
    struct wlr_subcompositor *subcomposior = wlr_subcompositor_create(core->wl_display);
    IVY_CHECK(subcomposior != NULL, "[WARNING] Failed to create wlr_subcompositor!");

    // Clipboard, Drag-and-drop
    struct wlr_data_device_manager *data_device_manager = wlr_data_device_manager_create(core->wl_display);
    IVY_CHECK(data_device_manager != NULL, "[WARNING] Failed to create wlr_data_device_manager!");
}

static void IvyServer_InitOutput(IvyServer *server)
{
    const IvyServerCore *core = &server->core;
    IvyServerOutput *output = &server->output;

    output->wlr_output_layout = wlr_output_layout_create(core->wl_display);
    IVY_CHECK(output->wlr_output_layout != NULL, "[WARNING] Failed to create wlr_output_layout!");

    Ivy_OutputManager_Init(&output->manager);
}

static void IvyServer_InitScene(IvyServer *server)
{
    const IvyServerOutput *output = &server->output;
    IvyServerScene *scene = &server->scene;

    scene->wlr_scene = wlr_scene_create();
    IVY_CHECK(scene->wlr_scene != NULL, "[WARNING] Failed to create wlr_scene!");

    scene->background = wlr_scene_tree_create(&scene->wlr_scene->tree);
    scene->bottom     = wlr_scene_tree_create(&scene->wlr_scene->tree);
    scene->toplevel   = wlr_scene_tree_create(&scene->wlr_scene->tree);
    scene->top        = wlr_scene_tree_create(&scene->wlr_scene->tree);
    scene->overlay    = wlr_scene_tree_create(&scene->wlr_scene->tree);

    scene->wlr_scene_output_layout = wlr_scene_attach_output_layout(scene->wlr_scene, output->wlr_output_layout);
}

static void IvyServer_InitShells(IvyServer *server)
{
    Ivy_XdgShell_Init(&server->shell.xdg_shell);

    Ivy_XdgTopLevelManager_Init(&server->shell.xdg_toplevel_manager);
    Ivy_XdgPopupManager_Init(&server->shell.xdg_popup_manager);
}

static void IvyServer_InitInput(IvyServer *server)
{
    Ivy_Seat_Init(&server->seat);

    // TODO: ....
    // Ivy_KeyboardManager_Create();
    // Ivy_Cursor_Create();
}

static void IvyServer_InitIdle(IvyServer *server)
{
    Ivy_Idle_Init(&server->idle);
}