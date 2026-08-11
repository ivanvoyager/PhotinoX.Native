#pragma once

#ifdef _WIN32

#include "Photino.Application.h"
#include "Photino.Application.NotificationDispatch.h"
#include "Photino.Strings.h"

#include "Dependencies/wintoastlib.h"

#include <string>

namespace PhotinoX::Native
{
    class WinToastHandler final : public WinToastLib::IWinToastHandler
    {
      private:
        const PhotinoApplication* app_;
        int notificationId_;
        void* callbackState_;

      public:
        WinToastHandler(const PhotinoApplication* app, int notificationId, void* callbackState)
            : app_(app),
              notificationId_(notificationId),
              callbackState_(callbackState)
        {
        }

        void toastActivated() const override // The user clicked in this toast
        {
            NotificationDispatch::ScheduleNotificationActivated(app_, notificationId_, callbackState_);
        }

        void toastActivated(int actionIndex) const override // The user clicked on action #actionIndex
        {
            NotificationDispatch::ScheduleNotificationActionActivated(app_, notificationId_, actionIndex, callbackState_);
        }

        void toastActivated(std::wstring response) const override // The user replied with response
        {
            NotificationDispatch::ScheduleNotificationInputActivated(app_, notificationId_, ToUtf8String(response), callbackState_);
        }

        void toastFailed() const override // Error showing current toast
        {
            NotificationDispatch::ScheduleNotificationFailed(app_, notificationId_, callbackState_);
        }

        void toastDismissed(WinToastDismissalReason reason) const override
        {
            NotificationDispatch::ScheduleNotificationDismissed(app_, notificationId_, ToDismissalReason(reason), callbackState_);
        }

      private:

        static PhotinoNotificationDismissalReason ToDismissalReason(WinToastDismissalReason reason)
        {
            switch (reason)
            {
            case UserCanceled: // The user dismissed this toast
                return PhotinoNotificationDismissalReason::UserCanceled;

            case ApplicationHidden: // The application hid the toast using ToastNotifier.hide()
                return PhotinoNotificationDismissalReason::ApplicationHidden;

            case TimedOut: // The toast has timed out
                return PhotinoNotificationDismissalReason::TimedOut;

            default: // Toast not activated
                return PhotinoNotificationDismissalReason::Unknown;
            }
        }
    };
} // namespace PhotinoX::Native

#endif