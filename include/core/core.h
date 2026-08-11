#ifndef IVY_CORE_H
#define IVY_CORE_H

#include "core/fwd.h"

#ifdef __cplusplus
extern "C" {
#endif

struct IvyCore {
    struct wl_display       *wl_display;
    struct wlr_backend      *wlr_backend;
    struct wlr_renderer     *wlr_renderer;
    struct wlr_allocator    *wlr_allocator;
    struct wlr_compositor   *wlr_compositor;

    const char              *socket;
};

void Ivy_Core_Init(IvyCore *core);
void Ivy_Core_Start(IvyCore *core);
void Ivy_Core_Destroy(const IvyCore *core);

#ifdef __cplusplus
}
#endif

#endif