#ifndef IVY_CORE_OUTPUT_H
#define IVY_CORE_OUTPUT_H

#include "core/fwd.h"

#include <wayland-server-core.h>
#include <wlr/types/wlr_output.h>
#include <wlr/types/wlr_scene.h>

#ifdef __cplusplus
extern "C" {
#endif

struct IvyOutput
{
    struct wlr_output       *wlr_output;
    struct wlr_scene_output *wlr_scene_output;

    struct wl_list          link;

    struct wl_listener      frame;
    struct wl_listener      destroy;
};

struct IvyOutputManager
{
    struct wl_list          outputs;
    struct wl_listener      new_output;
};

void Ivy_OutputManager_Init(IvyOutputManager *output_manager);

IvyOutput *Ivy_Output_Create(struct wlr_output *restrict wlr_output, IvyOutputManager *restrict manager);
void Ivy_Output_Destroy(IvyOutput *output);

#ifdef __cplusplus
}
#endif

#endif