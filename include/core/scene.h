#ifndef IVY_CORE_SCENE_H
#define IVY_CORE_SCENE_H

#include "core/fwd.h"

#ifdef __cplusplus
extern "C" {
#endif

struct IvyScene
{
    struct wlr_scene *wlr_scene;
    struct wlr_scene_output_layout *wlr_scene_output_layout;

    struct wlr_scene_tree *background;
    struct wlr_scene_tree *bottom;
    struct wlr_scene_tree *toplevel;
    struct wlr_scene_tree *top;
    struct wlr_scene_tree *overlay;
};

void Ivy_Scene_Init(IvyScene *scene);
void Ivy_Scene_Destroy(IvyScene *scene);

#ifdef __cplusplus
}
#endif

#endif