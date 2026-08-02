#ifndef IVY_FORWARD_H
#define IVY_FORWARD_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct IvyServer IvyServer;

typedef struct IvyOutputManager IvyOutputManager;
typedef struct IvyOutput IvyOutput;

typedef struct IvySeat IvySeat;
typedef struct IvyKeyboardManager IvyKeyboardManager;
typedef struct IvyKeyboard IvyKeyboard;
typedef struct IvyCursor IvyCursor;

typedef struct IvyIdle IvyIdle;
typedef struct IvyIdleInhibitor IvyIdleInhibitor;
typedef struct IvyIdleInhibitorManager IvyIdleInhibitorManager;

typedef struct IvyXdgShell IvyXdgShell;
typedef struct IvyXdgTopLevelManager IvyXdgTopLevelManager;
typedef struct IvyXdgTopLevel IvyXdgTopLevel;
typedef struct IvyXdgPopupManager IvyXdgPopupManager;
typedef struct IvyXdgPopup IvyXdgPopup;

typedef struct IvyLayerShell IvyLayerShell;
typedef struct IvyLayerSurfaceManager IvyLayerSurfaceManager;
typedef struct IvyLayerSurface IvyLayerSurface;

#ifdef __cplusplus
}
#endif

#endif
