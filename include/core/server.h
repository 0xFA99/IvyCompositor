#ifndef IVY_SERVER_H
#define IVY_SERVER_H

#include "core/fwd.h"
#include "core/output.h"
#include "shell/xdg_shell.h"
#include "shell/layer_shell.h"
#include "input/seat.h"
#include "input/idle_inhibitor.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    struct wl_display               *wl_display;
    struct wlr_backend              *wlr_backend;
    struct wlr_renderer             *wlr_renderer;
    struct wlr_allocator            *wlr_allocator;
    struct wlr_compositor           *wlr_compositor;
} IvyServerCore;

typedef struct {
    struct wlr_output_layout *wlr_output_layout;
    IvyOutputManager manager;
} IvyServerOutput;

typedef struct {
    struct wlr_scene                *wlr_scene;
    struct wlr_scene_output_layout  *wlr_scene_output_layout;

    // Scene Layers (Z-Index Tree)
    struct wlr_scene_tree           *background;
    struct wlr_scene_tree           *bottom;
    struct wlr_scene_tree           *toplevel;
    struct wlr_scene_tree           *top;
    struct wlr_scene_tree           *overlay;
} IvyServerScene;

typedef struct {
    IvyXdgShell xdg_shell;
    IvyLayerShell layer_shell;
} IvyServerShells;

struct IvyServer {
    IvyServerCore   core;
    IvyServerScene  scene;
    IvyServerOutput output;

    IvySeat         seat;

    IvyServerShells shell;
    IvyIdle         idle;
};

void Ivy_Server_Init(IvyServer *server);

#ifdef __cplusplus
}
#endif

#endif
