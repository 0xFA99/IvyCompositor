#ifndef IVY_TOP_LEVEL_COMMON_H
#define IVY_TOP_LEVEL_COMMON_H

#include "top_level.h"

#ifdef __cplusplus
extern "C" {
#endif

void IvyTopLevel_GetSize(IvyTopLevel *topLevel, int *width, int *height);
struct wlr_output *IvyTopLevel_FindOutputForBounds(IvyServer *server, int x, int y, int width, int height);
void IvyTopLevel_SetBordersEnabled(IvyTopLevel *topLevel, bool enabled);
void IvyTopLevel_SaveGeometry(IvyTopLevel *topLevel, int width, int height);
void IvyTopLevel_RestoreGeometry(IvyTopLevel *topLevel);
void IvyTopLevel_RemoveListenerIfLinked(struct wl_listener *listener);

void IvyTopLevel_HandleMap(struct wl_listener *listener, void *data);
void IvyTopLevel_HandleUnmap(struct wl_listener *listener, void *data);
void IvyTopLevel_HandleCommit(struct wl_listener *listener, void *data);

#ifdef __cplusplus
}
#endif

#endif