#ifndef TOASTHANDLER_H
#define TOASTHANDLER_H
#include "Photino.h"
#include "Dependencies/wintoastlib.h"
#include <WinUser.h>

using namespace WinToastLib;

class WinToastHandler final : public IWinToastHandler
{
private:
    Photino* window_;

public:
    explicit WinToastHandler(Photino* window) : window_(window)
    {
    }

    void toastActivated() const override
    {
        ShowWindow(this->window_->getHwnd(), SW_SHOW);    // Make the window visible if it was hidden
        ShowWindow(this->window_->getHwnd(), SW_RESTORE); // Next, restore it if it was minimized
        SetForegroundWindow(this->window_->getHwnd());    // Finally, activate the window
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
#endif