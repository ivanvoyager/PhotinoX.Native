#include "Photino.Export.h"
#include "Photino.Application.h"
#include "Photino.Application.Notifications.h"

using namespace PhotinoX::Native;

extern "C"
{
    // > 0 = notification id
    // 0   = not shown
    // -1  = failure
    PHOTINO_EXPORT int PhotinoApplication_ShowNotification(const PhotinoNotificationShowParams* showParams)
    {
        return PhotinoApplication::Instance().ShowNotification(showParams);
    }

    PHOTINO_EXPORT void PhotinoApplication_GetNotificationsEnabled(bool* enabled)
    {
        if (!enabled) return;

        PhotinoApplication::Instance().GetNotificationsEnabled(enabled);
    }

    PHOTINO_EXPORT void PhotinoApplication_SetNotificationsEnabled(const bool enabled)
    {
        PhotinoApplication::Instance().SetNotificationsEnabled(enabled);
    }
}