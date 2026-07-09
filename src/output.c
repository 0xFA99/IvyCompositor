#include "types.h"
#include "server.h"
#include "output.h"

#include <wlr/types/wlr_scene.h>
#include <wlr/types/wlr_output.h>
#include <wlr/types/wlr_output_layout.h>
#include <wlr/types/wlr_layer_shell_v1.h>
#include <wlr/xwayland.h>
#include <wlr/types/wlr_xdg_shell.h>
#include "layer_surface.h"
#include "top_level.h"

#include <stdlib.h>
#include <time.h>

static void IvyOutput_HandleFrame(struct wl_listener *listener, void *data)
{
    IvyOutput *output = wl_container_of(listener, output, frame);
    (void)data;

    struct wlr_scene_output *scene_output = wlr_scene_get_scene_output(output->server->scene, output->wlr_output);
    wlr_scene_output_commit(scene_output, NULL);

    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    wlr_scene_output_send_frame_done(scene_output, &now);
}

static void IvyOutput_HandleRequestState(struct wl_listener *listener, void *data)
{
    IvyOutput *output = wl_container_of(listener, output, request_state);
    const struct wlr_output_event_request_state *event = data;

    wlr_output_commit_state(output->wlr_output, event->state);
}

static void IvyOutput_HandleDestroy(struct wl_listener *listener, void *data)
{
    IvyOutput *output = wl_container_of(listener, output, destroy);
    (void)data;

    wl_list_remove(&output->frame.link);
    wl_list_remove(&output->request_state.link);
    wl_list_remove(&output->destroy.link);
    wl_list_remove(&output->link);

    free(output);
}

void Ivy_Server_HandleNewOutput(struct wl_listener *listener, void *data)
{
    IvyServer *server = wl_container_of(listener, server, new_output);
    struct wlr_output *wlr_output = data;

    wlr_output_init_render(wlr_output, server->allocator, server->renderer);

    struct wlr_output_state state;
    wlr_output_state_init(&state);
    wlr_output_state_set_enabled(&state, true);

    struct wlr_output_mode *mode = wlr_output_preferred_mode(wlr_output);
    if (mode != NULL) wlr_output_state_set_mode(&state, mode);

    wlr_output_commit_state(wlr_output, &state);
    wlr_output_state_finish(&state);

    IvyOutput *output = calloc(1, sizeof(IvyOutput));
    IVY_CHECK(output != NULL, "[WARNING] Failed to allocate IvyOutput!");

    output->server = server;
    output->wlr_output = wlr_output;
    wlr_output->data = output;

    wl_list_init(&output->layers);

    output->frame.notify = IvyOutput_HandleFrame;
    wl_signal_add(&wlr_output->events.frame, &output->frame);

    output->request_state.notify = IvyOutput_HandleRequestState;
    wl_signal_add(&wlr_output->events.request_state, &output->request_state);

    output->destroy.notify = IvyOutput_HandleDestroy;
    wl_signal_add(&wlr_output->events.destroy, &output->destroy);

    wl_list_insert(&server->outputs, &output->link);

    struct wlr_output_layout_output *layout_output = wlr_output_layout_add_auto(server->output_layout, wlr_output);
    struct wlr_scene_output *scene_output = wlr_scene_output_create(server->scene, wlr_output);

    wlr_scene_output_layout_add_output(server->scene_layout, layout_output, scene_output);

    output->usable_area.x = layout_output->x;
    output->usable_area.y = layout_output->y;
    output->usable_area.width = wlr_output->width;
    output->usable_area.height = wlr_output->height;
}

void Ivy_Output_ArrangeLayers(IvyOutput *output)
{
    IvyServer *server = output->server;
    struct wlr_output_layout_output *layout_output = wlr_output_layout_get(server->output_layout, output->wlr_output);
    if (!layout_output) return;

    struct wlr_box full_area = {
        .x = layout_output->x,
        .y = layout_output->y,
        .width = output->wlr_output->width,
        .height = output->wlr_output->height
    };

    output->usable_area = full_area;

    IvyLayerSurface *layer_surface;
    wl_list_for_each(layer_surface, &output->layers, link) {
        wlr_scene_layer_surface_v1_configure(layer_surface->scene_layer_surface, &full_area, &output->usable_area);
    }

    IvyTopLevel *topLevel;
    wl_list_for_each(topLevel, &server->topLevels, link) {
        if (topLevel->is_maximized && !topLevel->is_fullscreen) {
            double center_x, center_y;
            if (topLevel->type == IVY_TOPLEVEL_XDG) {
                center_x = topLevel->scene_tree->node.x + topLevel->xdg_toplevel->base->geometry.width * 0.5;
                center_y = topLevel->scene_tree->node.y + topLevel->xdg_toplevel->base->geometry.height * 0.5;
            } else {
                center_x = topLevel->scene_tree->node.x + topLevel->xwayland_surface->width * 0.5;
                center_y = topLevel->scene_tree->node.y + topLevel->xwayland_surface->height * 0.5;
            }

            struct wlr_output *wlr_output = wlr_output_layout_output_at(
                server->output_layout, center_x, center_y);
            
            if (wlr_output == output->wlr_output) {
                wlr_scene_node_set_position(&topLevel->scene_tree->node, output->usable_area.x, output->usable_area.y);
                if (topLevel->type == IVY_TOPLEVEL_XDG) {
                    wlr_xdg_toplevel_set_size(topLevel->xdg_toplevel, output->usable_area.width, output->usable_area.height);
                } else {
                    wlr_xwayland_surface_configure(topLevel->xwayland_surface,
                                                   output->usable_area.x, output->usable_area.y,
                                                   output->usable_area.width, output->usable_area.height);
                }
            }
        }
    }
}
