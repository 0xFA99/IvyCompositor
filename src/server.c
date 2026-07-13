#include "input.h"
#include "types.h"
#include "server.h"
#include "output.h"
#include "top_level.h"
#include "cursor.h"
#include "popup.h"
#include "layer_surface.h"

#include <wlr/backend.h>
#include <wlr/xwayland.h>
#include <wlr/render/allocator.h>
#include <wlr/render/wlr_renderer.h>
#include <wlr/types/wlr_compositor.h>
#include <wlr/types/wlr_subcompositor.h>
#include <wlr/types/wlr_data_device.h>
#include <wlr/types/wlr_output_layout.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/types/wlr_seat.h>
#include <wlr/types/wlr_layer_shell_v1.h>
#include <wlr/types/wlr_xdg_decoration_v1.h>
#include <wlr/types/wlr_output.h>
#include <wlr/types/wlr_idle_inhibit_v1.h>
#include <wlr/types/wlr_idle_notify_v1.h>
#include <wlr/types/wlr_output_power_management_v1.h>
#include <wlr/types/wlr_xdg_activation_v1.h>

#include <stdlib.h>
#include <unistd.h>

#define IVY_XDG_SHELL_VERSION 6

static const float TEST_BACKGROUND_COLOR[4] = { 0.1f, 0.1f, 0.15f, 1.0f };

typedef struct {
    struct wl_listener destroy;
    IvyServer *server;
} IvyIdleInhibitor;

static void IvyServer_SeatRequestCursor(struct wl_listener *listener, void *data)
{
    IvyServer *server = wl_container_of(listener, server, request_cursor);
    struct wlr_seat_pointer_request_set_cursor_event *event = data;
    struct wlr_seat_client *focused_client = server->seat->pointer_state.focused_client;

    if (focused_client == event->seat_client)
        Ivy_Cursor_SetSurface(server->cursor, event->surface, event->hotspot_x, event->hotspot_y);
}

static void IvyServer_SeatPointerFocusChange(struct wl_listener *listener, void *data)
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

static void IvyServer_UpdateIdleInhibited(IvyServer *server)
{
    const bool inhibited = !wl_list_empty(&server->idle_inhibit_manager->inhibitors);
    wlr_idle_notifier_v1_set_inhibited(server->idle_notifier, inhibited);
}

static void IvyServer_HandleIdleInhibitorDestroy(struct wl_listener *listener, void *data)
{
    IvyIdleInhibitor *inhibitor = wl_container_of(listener, inhibitor, destroy);
    IvyServer *server = inhibitor->server;

    wl_list_remove(&inhibitor->destroy.link);
    free(inhibitor);

    IvyServer_UpdateIdleInhibited(server);
}

static void IvyServer_HandleNewIdleInhibitor(struct wl_listener *listener, void *data)
{
    IvyServer *server = wl_container_of(listener, server, new_idle_inhabitor);
    struct wlr_idle_inhibitor_v1 *wlr_inhibitor = data;

    IvyIdleInhibitor *inhibitor = calloc(1, sizeof(IvyIdleInhibitor));
    inhibitor->server = server;
    inhibitor->destroy.notify = IvyServer_HandleIdleInhibitorDestroy;
    wl_signal_add(&wlr_inhibitor->events.destroy, &inhibitor->destroy);

    IvyServer_UpdateIdleInhibited(server);
}

static void IvyServer_HandleOutputPowerSetMode(struct wl_listener *listener, void *data)
{
    IvyServer *server = wl_container_of(listener, server, output_power_set_mode);
    struct wlr_output_power_v1_set_mode_event *event = data;

    struct wlr_output_state state;
    wlr_output_state_init(&state);

    const bool enable = (event->mode == ZWLR_OUTPUT_POWER_V1_MODE_ON);
    wlr_output_state_set_enabled(&state, enable);

    wlr_output_commit_state(event->output, &state);
    wlr_output_state_finish(&state);
}

static void IvyServer_HandleRequestActivate(struct wl_listener *listener, void *data)
{
    IvyServer *server = wl_container_of(listener, server, request_activate);
    struct wlr_xdg_activation_v1_request_activate_event *event = data;

    IvyTopLevel *topLevel = NULL;
    IvyTopLevel *iter;
    wl_list_for_each(iter, &server->topLevels, link) {
        struct wlr_surface *surface = (iter->type == IVY_TOPLEVEL_XDG)
            ? iter->xdg_toplevel->base->surface
            : iter->xwayland_surface->surface;
        if (surface == event->surface) {
            topLevel = iter;
            break;
        }
    }

    if (topLevel != NULL) {
        Ivy_TopLevel_Focus(topLevel);
    }
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

    server->scene_background = wlr_scene_tree_create(&server->scene->tree);
    server->scene_bottom = wlr_scene_tree_create(&server->scene->tree);
    server->scene_toplevel = wlr_scene_tree_create(&server->scene->tree);
    server->scene_top = wlr_scene_tree_create(&server->scene->tree);
    server->scene_overlay = wlr_scene_tree_create(&server->scene->tree);

    server->scene_layout = wlr_scene_attach_output_layout(server->scene, server->output_layout);
    IVY_CHECK(server->scene_layout != NULL, "[WARNING] Failed to attach scene output layout!");

    server->background = wlr_scene_rect_create(server->scene_background, 100000, 100000, TEST_BACKGROUND_COLOR);
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

    server->pointer_focus_change.notify = IvyServer_SeatPointerFocusChange;
    wl_signal_add(&server->seat->pointer_state.events.focus_change, &server->pointer_focus_change);

    server->request_set_selection.notify = IvyServer_SeatRequestSetSelection;
    wl_signal_add(&server->seat->events.request_set_selection, &server->request_set_selection);

    server->layer_shell = wlr_layer_shell_v1_create(server->wl_display, 4);
    IVY_CHECK(server->layer_shell != NULL, "[WARNING] Failed to create wlr_layer_shell_v1!");

    server->new_layer_surface.notify = Ivy_Server_HandleNewLayerSurface;
    wl_signal_add(&server->layer_shell->events.new_surface, &server->new_layer_surface);

    server->xwayland = wlr_xwayland_create(server->wl_display, compositor, true);
    if (server->xwayland != NULL) {
        server->new_xwayland_surface.notify = Ivy_Server_HandleNewXWaylandSurface;
        wl_signal_add(&server->xwayland->events.new_surface, &server->new_xwayland_surface);
        wlr_xwayland_set_seat(server->xwayland, server->seat);
        setenv("DISPLAY", server->xwayland->display_name, true);
    } else {
        IVY_CHECK(false, "[WARNING] Failed to create wlr_xwayland!");
    }

    server->xdg_decoration_manager = wlr_xdg_decoration_manager_v1_create(server->wl_display);
    if (server->xdg_decoration_manager != NULL) {
        server->new_xdg_decoration.notify = Ivy_Server_HandleNewXdgDecoration;
        wl_signal_add(&server->xdg_decoration_manager->events.new_toplevel_decoration, &server->new_xdg_decoration);
    } else {
        IVY_CHECK(false, "[WARNING] Failed to create wlr_xdg_decoration_manager_v1!");
    }

    server->idle_notifier = wlr_idle_notifier_v1_create(server->wl_display);
    IVY_CHECK(server->idle_notifier != NULL, "[WARNING] Failed to create wlr_idle_notifier_v1");

    server->idle_inhibit_manager = wlr_idle_inhibit_v1_create(server->wl_display);
    if (server->idle_inhibit_manager != NULL) {
        server->new_idle_inhabitor.notify = IvyServer_HandleNewIdleInhibitor;
        wl_signal_add(&server->idle_inhibit_manager->events.new_inhibitor, &server->new_idle_inhabitor);
    } else {
        IVY_CHECK(false, "[WARNING] Failed to create wlr_idle_inhibit_manager_v1");
    }

    server->output_power_manager = wlr_output_power_manager_v1_create(server->wl_display);
    if (server->output_power_manager != NULL) {
        server->output_power_set_mode.notify = IvyServer_HandleOutputPowerSetMode;
        wl_signal_add(&server->output_power_manager->events.set_mode, &server->output_power_set_mode);
    } else {
        IVY_CHECK(false, "[WARNING] Failed to create wlr_output_power_manager_v1");
    }

    server->xdg_activation = wlr_xdg_activation_v1_create(server->wl_display);
    if (server->xdg_activation != NULL) {
        server->request_activate.notify = IvyServer_HandleRequestActivate;
        wl_signal_add(&server->xdg_activation->events.request_activate, &server->request_activate);
    } else {
        IVY_CHECK(false, "[WARNING] Failed to create wlr_xdg_activation_v1");
    }

    server->current_workspace = 1;
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

void Ivy_Server_Destroy(IvyServer *server)
{
    IVY_ASSERT(server != NULL, "[ERROR] IvyServer is NULL!");

    wl_list_remove(&server->new_input.link);
    wl_list_remove(&server->new_output.link);
    wl_list_remove(&server->new_xdg_topLevel.link);
    wl_list_remove(&server->new_xdg_popup.link);
    wl_list_remove(&server->new_layer_surface.link);

    wl_list_remove(&server->request_cursor.link);
    wl_list_remove(&server->pointer_focus_change.link);
    wl_list_remove(&server->request_set_selection.link);

    if (server->xwayland != NULL) {
        wl_list_remove(&server->new_xwayland_surface.link);
        wlr_xwayland_destroy(server->xwayland);
    }

    if (server->xdg_decoration_manager != NULL) {
        wl_list_remove(&server->new_xdg_decoration.link);
    }

    if (server->xdg_activation != NULL) {
        wl_list_remove(&server->request_activate.link);
    }

    if (server->idle_inhibit_manager != NULL) wl_list_remove(&server->new_idle_inhabitor.link);
    if (server->output_power_manager != NULL) wl_list_remove(&server->output_power_set_mode.link);

    Ivy_Cursor_Destroy(server->cursor);

    wlr_scene_node_destroy(&server->scene->tree.node);
    wlr_allocator_destroy(server->allocator);
    wlr_renderer_destroy(server->renderer);
    wlr_backend_destroy(server->backend);

    wl_display_destroy(server->wl_display);
}

void Ivy_Server_SwitchWorkspace(IvyServer *server, int workspace)
{
    if (server->current_workspace == workspace) return;

    server->current_workspace = workspace;

    IvyTopLevel *topLevel;
    wl_list_for_each(topLevel, &server->topLevels, link) {
        if (topLevel->workspace == workspace) {
            wlr_scene_node_set_enabled(&topLevel->scene_tree->node, true);
        } else {
            wlr_scene_node_set_enabled(&topLevel->scene_tree->node, false);
        }
    }

    IvyTopLevel *next_focus = NULL;
    wl_list_for_each(topLevel, &server->topLevels, link) {
        if (topLevel->workspace == workspace) {
            next_focus = topLevel;
            break;
        }
    }

    if (next_focus != NULL) {
        Ivy_TopLevel_Focus(next_focus);
    } else {
        wlr_seat_pointer_clear_focus(server->seat);
        wlr_seat_keyboard_clear_focus(server->seat);
    }
}

void Ivy_TopLevel_MoveToWorkspace(IvyTopLevel *topLevel, int workspace)
{
    if (topLevel->workspace == workspace) return;

    IvyServer *server = topLevel->server;
    topLevel->workspace = workspace;

    if (workspace == server->current_workspace) {
        wlr_scene_node_set_enabled(&topLevel->scene_tree->node, true);
        Ivy_TopLevel_Focus(topLevel);
    } else {
        wlr_scene_node_set_enabled(&topLevel->scene_tree->node, false);
        struct wlr_surface *focused_surface = server->seat->keyboard_state.focused_surface;
        struct wlr_surface *surface = (topLevel->type == IVY_TOPLEVEL_XDG)
                                    ? topLevel->xdg_toplevel->base->surface
                                    : topLevel->xwayland_surface->surface;
        if (focused_surface == surface) {
            IvyTopLevel *next_focus = NULL;
            IvyTopLevel *tmp;
            wl_list_for_each(tmp, &server->topLevels, link) {
                if (tmp->workspace == server->current_workspace) {
                    next_focus = tmp;
                    break;
                }
            }
            if (next_focus != NULL) {
                Ivy_TopLevel_Focus(next_focus);
            } else {
                wlr_seat_pointer_clear_focus(server->seat);
                wlr_seat_keyboard_clear_focus(server->seat);
            }
        }
    }
}
