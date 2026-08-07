#include "core/server.h"
#include "core/output.h"

#include <wlr/backend.h>
#include <wlr/types/wlr_output_layout.h>
#include <wlr/types/wlr_output.h>

#include <stdlib.h>

static void IvyOutputManager_HandleNewOutput(struct wl_listener *listener, void *data);

static void IvyOutput_ConfigureState(struct wlr_output *wlr_output);
static IvyOutput *IvyOutput_Create(IvyServer *server, struct wlr_output *wlr_output);
static void IvyOutput_SetupSignals(IvyOutput *output);
static void IvyOutput_HandleFrame(struct wl_listener *listener, void *data);
static void IvyOutput_HandleRequestState(struct wl_listener *listener, void *data);
static void IvyOutput_HandleDestroy(struct wl_listener *listener, void *data);

void Ivy_OutputManager_Init(IvyOutputManager *output_manager)
{
    IvyServer *server = wl_container_of(output_manager, server, output.manager);

    server->output.wlr_output_layout = wlr_output_layout_create(server->core.wl_display);

    wl_list_init(&output_manager->outputs);

    output_manager->new_output.notify = IvyOutputManager_HandleNewOutput;
    wl_signal_add(&server->core.wlr_backend->events.new_output, &output_manager->new_output);
}

static void IvyOutputManager_HandleNewOutput(struct wl_listener *listener, void *data)
{
    IvyServer *server = wl_container_of(listener, server, output.manager.new_output);
    struct wlr_output *wlr_output = data;

    wlr_output_init_render(wlr_output, server->core.wlr_allocator, server->core.wlr_renderer);

    IvyOutput_ConfigureState(wlr_output);

    IvyOutput_Create(server, wlr_output);

    struct wlr_output_layout_output *layout_output = wlr_output_layout_add_auto(server->output.wlr_output_layout, wlr_output);
    struct wlr_scene_output *scene_output = wlr_scene_output_create(server->scene.wlr_scene, wlr_output);

    wlr_scene_output_layout_add_output(server->scene.wlr_scene_output_layout, layout_output, scene_output);
}

static void IvyOutput_ConfigureState(struct wlr_output *wlr_output)
{
    struct wlr_output_state state;
    wlr_output_state_init(&state);
    wlr_output_state_set_enabled(&state, true);

    struct wlr_output_mode *mode = wlr_output_preferred_mode(wlr_output);
    if (mode != NULL) wlr_output_state_set_mode(&state, mode);

    wlr_output_commit_state(wlr_output, &state);
    wlr_output_state_finish(&state);
}

static IvyOutput *IvyOutput_Create(IvyServer *server, struct wlr_output *wlr_output)
{
    IvyOutput *output = calloc(1, sizeof(IvyOutput));
    IVY_CHECK(output != NULL, "[WARNING] Failed to allocate IvyOutput!");

    output->server = server;
    output->wlr_output = wlr_output;

    wl_list_insert(&server->output.manager.outputs, &output->link);

    IvyOutput_SetupSignals(output);

    return output;
}

static void IvyOutput_SetupSignals(IvyOutput *output)
{
    output->frame.notify = IvyOutput_HandleFrame;
    wl_signal_add(&output->wlr_output->events.frame, &output->frame);

    output->request_state.notify = IvyOutput_HandleRequestState;
    wl_signal_add(&output->wlr_output->events.request_state, &output->request_state);

    output->destroy.notify = IvyOutput_HandleDestroy;
    wl_signal_add(&output->wlr_output->events.destroy, &output->destroy);
}

static void IvyOutput_HandleFrame(struct wl_listener *listener, void *data)
{
    IvyOutput *output = wl_container_of(listener, output, frame);
    struct wlr_scene *scene = output->server->scene.wlr_scene;

    struct wlr_scene_output *scene_output = wlr_scene_get_scene_output(scene, output->wlr_output);

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

    wl_list_remove(&output->frame.link);
    wl_list_remove(&output->request_state.link);
    wl_list_remove(&output->destroy.link);
    wl_list_remove(&output->link);

    free(output);
}
