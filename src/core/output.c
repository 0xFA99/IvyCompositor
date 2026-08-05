#include "core/types.h"
#include "core/server.h"
#include "core/output.h"

#include <wayland-server-core.h>
#include <wayland-util.h>
#include <wlr/backend.h>
#include <wlr/types/wlr_output.h>
#include <wlr/types/wlr_output_layout.h>
#include <wlr/types/wlr_scene.h>

#include <time.h>
#include <stdlib.h>

static void IvyOutputManager_HandleNewOutput(struct wl_listener *listener, void *data);
static void IvyOutput_HandleFrame(struct wl_listener *listener, void *data);
static void IvyOutput_HandleDestroy(struct wl_listener *listener, void *data);

void Ivy_OutputManager_Init(IvyOutputManager *output_manager)
{
    IVY_ASSERT(output_manager != NULL, "[ERROR] IvyOutputManager is NULL!");

    IvyServer *server = wl_container_of(output_manager, server, output.manager);

    wl_list_init(&output_manager->outputs);

    output_manager->new_output.notify = IvyOutputManager_HandleNewOutput;
    wl_signal_add(&server->core.wlr_backend->events.new_output, &output_manager->new_output);
}

IvyOutput *Ivy_Output_Create(struct wlr_output *restrict wlr_output, IvyOutputManager *restrict manager)
{
    IVY_ASSERT(wlr_output != NULL, "wlr_output is NULL!");
    IVY_ASSERT(manager != NULL, "IvyOutputManager is NULL!");

    IvyServerOutput *server_output = wl_container_of(manager, server_output, manager);
    IvyServer *server = wl_container_of(server_output, server, output);

    IvyOutput *output = calloc(1, sizeof(IvyOutput));
    IVY_CHECK(output != NULL, "[WARNING] Failed to allocate IvyOutput!");
    output->wlr_output = wlr_output;

    struct wlr_output_state state;
    wlr_output_state_init(&state);
    wlr_output_state_set_enabled(&state, true);

    struct wlr_output_mode *mode = wlr_output_preferred_mode(wlr_output);
    if (!mode && !wl_list_empty(&wlr_output->modes)) {
        struct wlr_output_mode *first_mode;
        wl_list_for_each(first_mode, &wlr_output->modes, link) {
            mode = first_mode;
            break;
        }
    }
    if (mode) {
        wlr_output_state_set_mode(&state, mode);
    }

    bool success = wlr_output_commit_state(wlr_output, &state);
    wlr_output_state_finish(&state);

    if (!success) {
        free(output);
        return NULL;
    }

    wl_list_insert(&manager->outputs, &output->link);

    struct wlr_output_layout_output *layout_output = wlr_output_layout_add_auto(server_output->wlr_output_layout, wlr_output);
    if (!layout_output) {
        wl_list_remove(&output->link);
        free(output);
        return NULL;
    }

    output->wlr_scene_output = wlr_scene_output_create(server->scene.wlr_scene, wlr_output);
    if (!output->wlr_scene_output) {
        wlr_output_layout_remove(server_output->wlr_output_layout, wlr_output);
        wl_list_remove(&output->link);
        free(output);
        return NULL;
    }

    wlr_scene_output_layout_add_output(server->scene.wlr_scene_output_layout, layout_output, output->wlr_scene_output);

    output->frame.notify = IvyOutput_HandleFrame;
    wl_signal_add(&wlr_output->events.frame, &output->frame);

    output->destroy.notify = IvyOutput_HandleDestroy;
    wl_signal_add(&wlr_output->events.destroy, &output->destroy);

    return output;
}

void Ivy_Output_Destroy(IvyOutput *output)
{
    if (!output) return;

    wl_list_remove(&output->frame.link);
    wl_list_remove(&output->destroy.link);
    wl_list_remove(&output->link);

    if (output->wlr_scene_output) {
        wlr_scene_output_destroy(output->wlr_scene_output);
        output->wlr_scene_output = NULL;
    }

    free(output);
}

static void IvyOutput_HandleFrame(struct wl_listener *listener, void *data)
{
    IvyOutput *output = wl_container_of(listener, output, frame);
    (void)data;

    wlr_scene_output_commit(output->wlr_scene_output, NULL);

    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    wlr_scene_output_send_frame_done(output->wlr_scene_output, &now);
}

static void IvyOutput_HandleDestroy(struct wl_listener *listener, void *data)
{
    IvyOutput *output = wl_container_of(listener, output, destroy);
    (void)data;

    Ivy_Output_Destroy(output);
}

static void IvyOutputManager_HandleNewOutput(struct wl_listener *listener, void *data)
{
    IvyOutputManager *manager = wl_container_of(listener, manager, new_output);
    struct wlr_output *wlr_output = data;
    const IvyOutput *output = Ivy_Output_Create(wlr_output, manager);
    (void)output;
}