#include "input.h"
#include "types.h"
#include "server.h"
#include "output.h"
#include "top_level.h"
#include "cursor.h"
#include "popup.h"

#include <wlr/backend.h>
#include <wlr/render/allocator.h>
#include <wlr/render/wlr_renderer.h>
#include <wlr/types/wlr_compositor.h>
#include <wlr/types/wlr_subcompositor.h>
#include <wlr/types/wlr_data_device.h>
#include <wlr/types/wlr_output_layout.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/types/wlr_seat.h>

#include <stdlib.h>
#include <unistd.h>

#define IVY_XDG_SHELL_VERSION 6

static const float TEST_BACKGROUND_COLOR[4] = { 0.1f, 0.1f, 0.15f, 1.0f };

static void IvyServer_SeatRequestCursor(struct wl_listener *listener, void *data)
{
    IvyServer *server = wl_container_of(listener, server, request_cursor);
    struct wlr_seat_pointer_request_set_cursor_event *event = data;
    struct wlr_seat_client *focused_client = server->seat->pointer_state.focused_client;

    if (focused_client == event->seat_client)
        Ivy_Cursor_SetSurface(server->cursor, event->surface, event->hotspot_x, event->hotspot_y);
}

static void IvyServer_SeatPointerFocusChance(struct wl_listener *listener, void *data)
{
    IvyServer *server = wl_container_of(listener, server, pointer_focus_change);
    struct wlr_seat_pointer_focus_change_event *event = data;

    if (event->new_surface == NULL)
        Ivy_Cursor_ResetImage(server->cursor);
}

static void IvyServer_SeatRequestSetSelection(struct wl_listener *listener, void *data)
{
    IvyServer *server = wl_container_of(listener, server, request_set_selection);
    struct wlr_seat_request_set_selection_event *event = data;

    wlr_seat_set_selection(server->seat, event->source, event->serial);
}

void Ivy_Server_Init(IvyServer *server)
{
    IVY_ASSERT(server != NULL, "[ERROR] IvyServer is NULL!");

    server->wl_display = wl_display_create();
    IVY_CHECK(server->wl_display != NULL, "[WARNING] Failed to create wl_display!");

    server->backend = wlr_backend_autocreate(wl_display_get_event_loop(server->wl_display), NULL);
    IVY_CHECK(server->backend != NULL, "[WARNING] Failed to create wlr_backend!");

    server->renderer = wlr_renderer_autocreate(server->backend);
    IVY_CHECK(server->renderer != NULL, "[WARNING] Failed to create wlr_renderer!");

    wlr_renderer_init_wl_display(server->renderer, server->wl_display);

    server->allocator = wlr_allocator_autocreate(server->backend, server->renderer);
    IVY_CHECK(server->allocator != NULL, "[WARNING] Failed to create wlr_allocator!");

    server->output_layout = wlr_output_layout_create(server->wl_display);
    IVY_CHECK(server->output_layout != NULL, "[WARNING] Failed to create wlr_output_layout!");

    server->scene = wlr_scene_create();
    IVY_CHECK(server->scene != NULL, "[WARNING] Failed to create wlr_scene!");

    server->scene_layout = wlr_scene_attach_output_layout(server->scene, server->output_layout);
    IVY_CHECK(server->scene_layout != NULL, "[WARNING] Failed to attach scene output layout!");

    server->background = wlr_scene_rect_create(&server->scene->tree, 100000, 100000, TEST_BACKGROUND_COLOR);
    IVY_CHECK(server->background != NULL, "[WARNING] Failed to create background rect!");

    wlr_scene_node_set_position(&server->background->node, -50000, -50000);

    wl_list_init(&server->outputs);
    server->new_output.notify = Ivy_Server_HandleNewOutput;
    wl_signal_add(&server->backend->events.new_output, &server->new_output);

    struct wlr_compositor *compositor = wlr_compositor_create(server->wl_display, 5, server->renderer);
    IVY_CHECK(compositor != NULL, "[WARNING] Failed to create wlr_compositor!");

    struct wlr_subcompositor *subcompositor = wlr_subcompositor_create(server->wl_display);
    IVY_CHECK(subcompositor != NULL, "[WARNING] Failed to create wlr_subcompositor!");

    struct wlr_data_device_manager *data_device_manager = wlr_data_device_manager_create(server->wl_display);
    IVY_CHECK(data_device_manager != NULL, "[WARNING] Failed to create wlr_data_device_manager!");

    server->xdg_shell = wlr_xdg_shell_create(server->wl_display, IVY_XDG_SHELL_VERSION);
    IVY_CHECK(server->xdg_shell != NULL, "[WARNING] Failed to create wlr_xdg_shell!");

    wl_list_init(&server->topLevels);
    server->new_xdg_topLevel.notify = Ivy_Server_HandleNewXdgTopLevel;
    wl_signal_add(&server->xdg_shell->events.new_toplevel, &server->new_xdg_topLevel);

    server->new_xdg_popup.notify = Ivy_Server_HandleNewXdgPopup;
    wl_signal_add(&server->xdg_shell->events.new_popup, &server->new_xdg_popup);

    server->seat = wlr_seat_create(server->wl_display, "seat0");
    IVY_CHECK(server->seat != NULL, "[WARNING] Failed to create wlr_seat!");

    wl_list_init(&server->keyboards);
    server->new_input.notify = Ivy_Server_HandleNewInput;
    wl_signal_add(&server->backend->events.new_input, &server->new_input);

    server->cursor = Ivy_Cursor_Create(server);

    server->request_cursor.notify = IvyServer_SeatRequestCursor;
    wl_signal_add(&server->seat->events.request_set_cursor, &server->request_cursor);

    server->pointer_focus_change.notify = IvyServer_SeatPointerFocusChance;
    wl_signal_add(&server->seat->pointer_state.events.focus_change, &server->pointer_focus_change);

    server->request_set_selection.notify = IvyServer_SeatRequestSetSelection;
    wl_signal_add(&server->seat->events.request_set_selection, &server->request_set_selection);
}

void Ivy_Server_Run(const IvyServer *restrict server, const char *restrict cmd)
{
    IVY_ASSERT(server != NULL, "[ERROR] IvyServer is NULL!");
    (void)cmd;

    const char *socket = wl_display_add_socket_auto(server->wl_display);
    IVY_CHECK(socket != NULL, "[WARNING] Failed to create Wayland socket!");

    bool started = wlr_backend_start(server->backend);
    IVY_CHECK(started, "[WARNING] Failed to start backend!");

    setenv("WAYLAND_DISPLAY", socket, true);

    wl_display_run(server->wl_display);
}

void Ivy_Server_Destroy(const IvyServer *server)
{
    IVY_ASSERT(server != NULL, "[ERROR] IvyServer is NULL!");

    wl_list_remove(&((IvyServer *)server)->new_input.link);
    wl_list_remove(&((IvyServer *)server)->new_output.link);
    wl_list_remove(&((IvyServer *)server)->new_xdg_topLevel.link);
    wl_list_remove(&((IvyServer *)server)->new_xdg_popup.link);

    wl_list_remove(&((IvyServer *)server)->request_cursor.link);
    wl_list_remove(&((IvyServer *)server)->pointer_focus_change.link);
    wl_list_remove(&((IvyServer *)server)->request_set_selection.link);

    Ivy_Cursor_Destroy(server->cursor);

    wlr_scene_node_destroy(&server->scene->tree.node);
    wlr_allocator_destroy(server->allocator);
    wlr_renderer_destroy(server->renderer);
    wlr_backend_destroy(server->backend);

    wl_display_destroy(server->wl_display);
}
