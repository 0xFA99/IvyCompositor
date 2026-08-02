#ifndef IVY_INPUT_IDLE_H
#define IVY_INPUT_IDLE_H

#include "core/fwd.h"

#include <wayland-server-core.h>
#include <wayland-util.h>

struct wlr_idle_notifier_v1;
struct wlr_idle_inhibitor_v1;
struct wlr_idle_inhibit_manager_v1;

#ifdef __cplusplus
extern "C" {
#endif

struct IvyIdleInhibitor
{
    IvyIdleInhibitorManager *manager;
    struct wlr_idle_inhibitor_v1 *wlr_idle_inhibitor;
    struct wl_listener destroy;
    struct wl_list link;
};

struct IvyIdleInhibitorManager
{
    IvyIdle *idle;
    struct wlr_idle_inhibit_manager_v1 *wlr_idle_inhibit_manager;
    struct wl_listener new_idle_inhibitor;
    struct wl_list inhibitors;
};

struct IvyIdle
{
    struct wlr_idle_notifier_v1 *wlr_idle_notifier;
    IvyIdleInhibitorManager idle_inhibitor_manager;
};

void Ivy_Idle_Init(IvyIdle *idle);
void Ivy_Idle_Destroy(IvyIdle *idle);

#ifdef __cplusplus
}
#endif

#endif