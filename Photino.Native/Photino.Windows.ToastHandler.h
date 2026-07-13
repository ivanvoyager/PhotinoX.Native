#pragma once

#include "Photino.h"
#include "Dependencies/wintoastlib.h"
#include <WinUser.h>

class WinToastHandler final : public WinToastLib::IWinToastHandler
{
private:
    PhotinoX::Native::Photino* photino_;

public:
    explicit WinToastHandler(PhotinoX::Native::Photino* photino) : photino_(photino)
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
