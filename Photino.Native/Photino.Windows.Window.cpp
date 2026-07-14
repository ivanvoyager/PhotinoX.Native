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

void Photino::GetPosition(int* x, int* y) const
{
    assert((x || y) && _hWnd);

    if (!x && !y) return;

    if (x) *x = 0;
    if (y) *y = 0;

    if (!_hWnd) return;

    RECT rect{};
    if (!GetWindowRect(_hWnd, &rect)) return;

    if (x) *x = rect.left;
    if (y) *y = rect.top;
}

void Photino::SetPosition(const int x, const int y)
{
    assert(_hWnd);
    if (!_hWnd) return;

    BOOL result = SetWindowPos(_hWnd, nullptr, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
    assert(result);
}

void Photino::GetSize(int* width, int* height) const
{
    assert((width || height) && _hWnd);
    if (!width && !height) return;

    if (width) *width = 0;
    if (height) *height = 0;

    if (!_hWnd) return;

    RECT rect{};
    if (!GetWindowRect(_hWnd, &rect)) return;

    if (width) *width = rect.right - rect.left;
    if (height) *height = rect.bottom - rect.top;
}

void Photino::SetSize(const int width, const int height)
{
    assert(_hWnd);
    if (!_hWnd) return;

    if (width <= 0 || height <= 0)
        return;

    int newWidth = width;
    int newHeight = height;

    if (_sizeLimits.minWidth > 0 && newWidth < _sizeLimits.minWidth)    newWidth = _sizeLimits.minWidth;
    if (_sizeLimits.minHeight > 0 && newHeight < _sizeLimits.minHeight) newHeight = _sizeLimits.minHeight;
    if (_sizeLimits.maxWidth > 0 && newWidth > _sizeLimits.maxWidth)    newWidth = _sizeLimits.maxWidth;
    if (_sizeLimits.maxHeight > 0 && newHeight > _sizeLimits.maxHeight) newHeight = _sizeLimits.maxHeight;

    BOOL result = SetWindowPos(_hWnd, nullptr, 0, 0, newWidth, newHeight, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
    assert(result);
}

void Photino::SetMinSize(const int width, const int height)
{
    _sizeLimits.minWidth = (std::max)(0, width);
    _sizeLimits.minHeight = (std::max)(0, height);

    if (_sizeLimits.maxWidth > 0 && _sizeLimits.minWidth > _sizeLimits.maxWidth)
        _sizeLimits.maxWidth = _sizeLimits.minWidth;
    if (_sizeLimits.maxHeight > 0 && _sizeLimits.minHeight > _sizeLimits.maxHeight)
        _sizeLimits.maxHeight = _sizeLimits.minHeight;

    assert(_hWnd);
    if (!_hWnd) return;

    int currWidth = 0, currHeight = 0;
    GetSize(&currWidth, &currHeight);

    int newWidth = currWidth < _sizeLimits.minWidth ? _sizeLimits.minWidth : currWidth;
    int newHeight = currHeight < _sizeLimits.minHeight ? _sizeLimits.minHeight : currHeight;

    if (newWidth != currWidth || newHeight != currHeight)
        SetSize(newWidth, newHeight);
}

void Photino::SetMaxSize(const int width, const int height)
{
    _sizeLimits.maxWidth = (std::max)(0, width);
    _sizeLimits.maxHeight = (std::max)(0, height);

    if (_sizeLimits.maxWidth > 0 && _sizeLimits.maxWidth < _sizeLimits.minWidth)
        _sizeLimits.minWidth = _sizeLimits.maxWidth;

    if (_sizeLimits.maxHeight > 0 && _sizeLimits.maxHeight < _sizeLimits.minHeight)
        _sizeLimits.minHeight = _sizeLimits.maxHeight;

    assert(_hWnd);
    if (!_hWnd) return;

    int currWidth = 0, currHeight = 0;
    GetSize(&currWidth, &currHeight);

    int newWidth = _sizeLimits.maxWidth > 0 && currWidth > _sizeLimits.maxWidth ? _sizeLimits.maxWidth : currWidth;
    int newHeight = _sizeLimits.maxHeight > 0 && currHeight > _sizeLimits.maxHeight ? _sizeLimits.maxHeight : currHeight;

    if (newWidth != currWidth || newHeight != currHeight)
        SetSize(newWidth, newHeight);
}