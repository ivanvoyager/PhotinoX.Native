#include "Photino.h"

#include "Dependencies/wintoastlib.h"
#include <WinUser.h>

#include <cassert>

using namespace WinToastLib;
using namespace PhotinoX::Native;

void Photino::Close() const
{
    assert(_hWnd);
    if (!_hWnd) return;

    BOOL result = PostMessageW(_hWnd, WM_CLOSE, 0, 0);
    assert(result);
}

void Photino::SetTitle(const PlatformString& title)
{
    assert(_hWnd);
    if (!_hWnd) return;

    if (!SetWindowTextW(_hWnd, title.c_str()))
        return;

    _windowTitle = title;

    if (_notificationsEnabled)
    {
        WinToast::instance()->setAppName(title);

        if (_notificationRegistrationId.empty())
            WinToast::instance()->setAppUserModelId(title);
    }
}


void Photino::SetIconFile(const PlatformString& filename)
{
    assert(_hWnd);
    if (!_hWnd || filename.empty())
        return;

    HICON iconSmall = static_cast<HICON>(LoadImageW(
        nullptr,
        filename.c_str(),
        IMAGE_ICON,
        GetSystemMetrics(SM_CXSMICON),
        GetSystemMetrics(SM_CYSMICON),
        LR_LOADFROMFILE | LR_SHARED));

    HICON iconBig = static_cast<HICON>(LoadImageW(
        nullptr,
        filename.c_str(),
        IMAGE_ICON,
        GetSystemMetrics(SM_CXICON),
        GetSystemMetrics(SM_CYICON),
        LR_LOADFROMFILE | LR_SHARED));

    if (!iconSmall && !iconBig)
        return;

    _iconFileName = filename;

    if (iconSmall)
        SendMessageW(_hWnd, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(iconSmall));

    if (iconBig)
        SendMessageW(_hWnd, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(iconBig));
}