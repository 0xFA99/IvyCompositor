#include "core/fwd.h"
#include "core/types.h"
#include "core/server.h"
#include "core/scene.h"

#include <wlr/types/wlr_scene.h>

void Ivy_Scene_Init(IvyScene *scene)
{
    IVY_ASSERT(scene != NULL, "[ERROR] IvyScene is NULL!");

    IvyServer *server = wl_container_of(scene, server, scene);

    scene->wlr_scene = wlr_scene_create();
    IVY_CHECK(scene->wlr_scene != NULL, "[WARNING] Failed to create wlr_scene!");

    scene->background = wlr_scene_tree_create(&scene->wlr_scene->tree);
    scene->bottom     = wlr_scene_tree_create(&scene->wlr_scene->tree);
    scene->toplevel   = wlr_scene_tree_create(&scene->wlr_scene->tree);
    scene->top        = wlr_scene_tree_create(&scene->wlr_scene->tree);
    scene->overlay    = wlr_scene_tree_create(&scene->wlr_scene->tree);

    scene->wlr_scene_output_layout = wlr_scene_attach_output_layout(scene->wlr_scene, server->output_manager.wlr_output_layout);
}
