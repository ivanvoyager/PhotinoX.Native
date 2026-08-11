#include "Photino.Export.h"
#include "Photino.Callbacks.h"
#include "Photino.Application.h"

using namespace PhotinoX::Native;

extern "C"
{
    PHOTINO_EXPORT void PhotinoApplication_SetStartupCallback(const StartupCallback callback)
    {
        PhotinoApplication::Instance().SetStartupCallback(callback);
    }

    PHOTINO_EXPORT void PhotinoApplication_SetExitCallback(const ExitCallback callback)
    {
        PhotinoApplication::Instance().SetExitCallback(callback);
    }

    PHOTINO_EXPORT void PhotinoApplication_SetNotificationActivatedCallback(const NotificationActivatedCallback callback)
    {
        PhotinoApplication::Instance().SetNotificationActivatedCallback(callback);
    }

    PHOTINO_EXPORT void PhotinoApplication_SetNotificationActionActivatedCallback(const NotificationActionActivatedCallback callback)
    {
        PhotinoApplication::Instance().SetNotificationActionActivatedCallback(callback);
    }

    PHOTINO_EXPORT void PhotinoApplication_SetNotificationInputActivatedCallback(const NotificationInputActivatedCallback callback)
    {
        PhotinoApplication::Instance().SetNotificationInputActivatedCallback(callback);
    }

    PHOTINO_EXPORT void PhotinoApplication_SetNotificationDismissedCallback(const NotificationDismissedCallback callback)
    {
        PhotinoApplication::Instance().SetNotificationDismissedCallback(callback);
    }

    PHOTINO_EXPORT void PhotinoApplication_SetNotificationFailedCallback(const NotificationFailedCallback callback)
    {
        PhotinoApplication::Instance().SetNotificationFailedCallback(callback);
    }
}