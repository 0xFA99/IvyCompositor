#ifndef IVY_CORE_OUTPUT_H
#define IVY_CORE_OUTPUT_H

#include "core/fwd.h"

#include <wayland-server-core.h>

#ifdef __cplusplus
extern "C" {
#endif

struct IvyOutput {
    struct wlr_output *wlr_output;
};

struct IvyOutputManager {
    struct wl_list list;
    struct wl_listener new_output;
};

void Ivy_OutputManager_Init(IvyOutputManager *output_manager);

void Ivy_Output_Create(void);
void Ivy_Output_Init(IvyOutput *output);

#ifdef __cplusplus
}
#endif

#endif