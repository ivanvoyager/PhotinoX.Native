#pragma once

#include "Dependencies/wintoastlib.h"
#include "Photino.h"
#include <WinUser.h>

namespace PhotinoX::Native
{
    class WinToastHandler final : public WinToastLib::IWinToastHandler
    {
      private:
        Photino* photino_;

      public:
        explicit WinToastHandler(Photino* photino) : photino_(photino)
        {
        }

        void toastActivated() const override
        {
            ShowWindow(photino_->GetHwnd(), SW_SHOW);    // Make the window visible if it was hidden
            ShowWindow(photino_->GetHwnd(), SW_RESTORE); // Next, restore it if it was minimized
            SetForegroundWindow(photino_->GetHwnd());    // Finally, activate the window
        }

        void toastActivated(int actionIndex) const override
        {
            //
        }

        void toastActivated(std::wstring response) const override
        {
            //
        }

        void toastDismissed(WinToastDismissalReason state) const override
        {
            //
        }

        void toastFailed() const override
        {
            //
        }
    };
} // namespace PhotinoX::Native
