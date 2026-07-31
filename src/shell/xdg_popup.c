#include "core/fwd.h"
#include "core/types.h"
#include "shell/xdg_popup.h"

void Ivy_XdgPopupManager_Init(IvyXdgPopupManager *popup_manager)
{
    IVY_ASSERT(popup_manager != NULL, "[ERROR] IvyPopupManager is NULL!");
    wl_list_init(&popup_manager->popups);
}

void Ivy_Popup_Init(IvyXdgPopup *popup)
{
    IVY_ASSERT(popup != NULL, "[ERROR] IvyPopup is NULL!");

}