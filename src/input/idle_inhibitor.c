#include "core/fwd.h"
#include "core/types.h"
#include "core/server.h"
#include "input/idle_inhibitor.h"

#include <wayland-server-core.h>
#include <wayland-util.h>
#include <wlr/types/wlr_idle_notify_v1.h>
#include <wlr/types/wlr_idle_inhibit_v1.h>

#include <stdlib.h>

static void IvyIdle_UpdateInhibited(IvyIdle *idle);

static void IvyIdle_HandleNewIdleInhibitor(struct wl_listener *listener, void *data);
static void IvyIdle_HandleInhibitorDestroy(struct wl_listener *listener, void *data);

void Ivy_Idle_Init(IvyIdle *idle)
{
    IVY_ASSERT(idle != NULL, "[ERROR] IvyIdle is NULL!");

    IvyServer *server = wl_container_of(idle, server, idle);
    idle->wlr_idle_notifier = wlr_idle_notifier_v1_create(server->core.wl_display);
    IVY_CHECK(idle->wlr_idle_notifier != NULL, "[WARNING] Failed to create wlr_idle_notifier_v1!");

    IvyIdleInhibitorManager *manager = &idle->idle_inhibitor_manager;
    manager->idle = idle;
    wl_list_init(&manager->inhibitors);

    manager->wlr_idle_inhibit_manager = wlr_idle_inhibit_v1_create(server->core.wl_display);
    IVY_CHECK(manager->wlr_idle_inhibit_manager != NULL, "[WARNING] Failed to create wlr_idle_inhibitor_manager!");

    manager->new_idle_inhibitor.notify = IvyIdle_HandleNewIdleInhibitor;
    wl_signal_add(&manager->wlr_idle_inhibit_manager->events.new_inhibitor, &manager->new_idle_inhibitor);
}

void Ivy_Idle_Destroy(IvyIdle *idle)
{
    IVY_ASSERT(idle != NULL, "[ERROR] IvyIdle is NULL!");

    IvyIdleInhibitorManager *manager = &idle->idle_inhibitor_manager;

    if (manager->wlr_idle_inhibit_manager) {
        wl_list_remove(&manager->new_idle_inhibitor.link);
    }

    IvyIdleInhibitor *inhibitor, *temp;
    wl_list_for_each_safe(inhibitor, temp, &manager->inhibitors, link)
    {
        wl_list_remove(&inhibitor->destroy.link);
        wl_list_remove(&inhibitor->link);

        free(inhibitor);
    }
}

static void IvyIdle_UpdateInhibited(IvyIdle *idle)
{
    const bool inhibited = !wl_list_empty(&idle->idle_inhibitor_manager.inhibitors);
    wlr_idle_notifier_v1_set_inhibited(idle->wlr_idle_notifier, inhibited);
}

static void IvyIdle_HandleNewIdleInhibitor(struct wl_listener *listener, void *data)
{
    IvyIdleInhibitorManager *manager = wl_container_of(listener, manager, new_idle_inhibitor);
    struct wlr_idle_inhibitor_v1 *wlr_inhibitor = data;

    IvyIdleInhibitor *inhibitor = calloc(1, sizeof(IvyIdleInhibitor));
    IVY_CHECK(inhibitor != NULL, "[WARNING] Failed to allocate IvyIdleInhibitor!");

    inhibitor->manager = manager;
    inhibitor->wlr_idle_inhibitor = wlr_inhibitor;

    inhibitor->destroy.notify = IvyIdle_HandleInhibitorDestroy;
    wl_signal_add(&wlr_inhibitor->events.destroy, &inhibitor->destroy);

    wl_list_insert(&manager->inhibitors, &inhibitor->link);

    IvyIdle_UpdateInhibited(manager->idle);
}

static void IvyIdle_HandleInhibitorDestroy(struct wl_listener *listener, void *data)
{
    IvyIdleInhibitor *inhibitor = wl_container_of(listener, inhibitor, destroy);
    IvyIdleInhibitorManager *manager = inhibitor->manager;

    wl_list_remove(&inhibitor->destroy.link);
    wl_list_remove(&inhibitor->link);

    free(inhibitor);

    IvyIdle_UpdateInhibited(manager->idle);
}
