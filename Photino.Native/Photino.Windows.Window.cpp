#include "Photino.h"
#include "Photino.Callbacks.h"
#include "Photino.Strings.h"

#include "Dependencies/wintoastlib.h"
#include <WinUser.h>
#include <Shellscalingapi.h>

#include <cassert>

#pragma comment(lib, "Shcore.lib")//TODO remove

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

void Photino::Center() const
{
    assert(_hWnd);
    if (!_hWnd) return;

    HMONITOR monitor = MonitorFromWindow(_hWnd, MONITOR_DEFAULTTONEAREST);
    MONITORINFO monitorInfo{sizeof(monitorInfo)};
    if (!GetMonitorInfoW(monitor, &monitorInfo)) return;

    RECT windowRect{};
    if (!GetWindowRect(_hWnd, &windowRect)) return;

    int windowWidth = windowRect.right - windowRect.left;
    int windowHeight = windowRect.bottom - windowRect.top;

    const RECT& work = monitorInfo.rcWork;

    int left = work.left + (work.right - work.left - windowWidth) / 2;
    int top = work.top + (work.bottom - work.top - windowHeight) / 2;

    BOOL result = SetWindowPos(_hWnd, nullptr, left, top, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
    assert(result);
}

void Photino::Restore() const
{
    assert(_hWnd);
    if (!_hWnd) return;

    ShowWindow(_hWnd, SW_RESTORE);
}

unsigned int Photino::GetScreenDpi() const
{
    assert(_hWnd);
    if (!_hWnd) return 96;

    UINT dpi = GetDpiForWindow(_hWnd);
    return dpi ? dpi : 96;
}

// https://learn.microsoft.com/en-us/windows/win32/api/winuser/nc-winuser-monitorenumproc
// To continue the enumeration, return TRUE.
// To stop the enumeration, return FALSE.
BOOL MonitorEnum(const HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, const LPARAM dwData)
{
    auto callback = reinterpret_cast<GetAllMonitorsCallback>(dwData);
    if (!callback) return FALSE;

    MONITORINFO info{};
    info.cbSize = sizeof(info);
    if (!GetMonitorInfoW(hMonitor, &info)) return TRUE;

    UINT dpiX = 96, dpiY = 96;
    if (FAILED(GetDpiForMonitor(hMonitor, MDT_EFFECTIVE_DPI, &dpiX, &dpiY)))
        dpiX = dpiY = 96;

    Monitor props{};
    props.monitor.x = info.rcMonitor.left;
    props.monitor.y = info.rcMonitor.top;
    props.monitor.width = info.rcMonitor.right - info.rcMonitor.left;
    props.monitor.height = info.rcMonitor.bottom - info.rcMonitor.top;
    props.work.x = info.rcWork.left;
    props.work.y = info.rcWork.top;
    props.work.width = info.rcWork.right - info.rcWork.left;
    props.work.height = info.rcWork.bottom - info.rcWork.top;
    props.scale = static_cast<double>(dpiY) / 96.0;

    return callback(&props) ? TRUE : FALSE;
}

void Photino::GetAllMonitors(GetAllMonitorsCallback callback) const noexcept
{
    assert(callback);
    if (!callback) return;

    BOOL result = EnumDisplayMonitors(nullptr, nullptr, MonitorEnum, reinterpret_cast<LPARAM>(callback));
    assert(result);
}

void Photino::GetFullScreen(bool* fullScreen) const
{
    assert(fullScreen);
    if (!fullScreen)
        return;

    *fullScreen = _fullScreen;
}

void Photino::SetFullScreen(const bool fullScreen)
{
    assert(_hWnd);
    if (!_hWnd) return;

    _fullScreen = fullScreen;
    LONG_PTR style = GetWindowLongPtrW(_hWnd, GWL_STYLE);
    if (fullScreen)
    {
        style |= WS_POPUP;
        style &= ~WS_OVERLAPPEDWINDOW;
    }
    else
    {
        if (_chromeless)
        {
            style |= WS_POPUP;
            style &= ~WS_OVERLAPPEDWINDOW;
        }
        else
        {
            style |= WS_OVERLAPPEDWINDOW;
            style &= ~WS_POPUP;
        }
    }

    SetLastError(0);
    LONG_PTR previousStyle = SetWindowLongPtrW(_hWnd, GWL_STYLE, style);
    assert(previousStyle != 0 || GetLastError() == ERROR_SUCCESS);

    HMONITOR monitor = MonitorFromWindow(_hWnd, MONITOR_DEFAULTTONEAREST);
    MONITORINFO monitorInfo{sizeof(monitorInfo)};

    if (GetMonitorInfoW(monitor, &monitorInfo))
    {
        const RECT& rc = fullScreen
                             ? monitorInfo.rcMonitor
                             : monitorInfo.rcWork;

        HRESULT hr = SetWindowPos(_hWnd, HWND_TOP, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, SWP_FRAMECHANGED);
        assert(SUCCEEDED(hr));
    }
    else
    {
        HRESULT hr = SetWindowPos(_hWnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED);
        assert(SUCCEEDED(hr));
    }
}

void Photino::GetMaximized(bool* isMaximized) const
{
    assert(isMaximized && _hWnd);
    if (!isMaximized) return;

    *isMaximized = false;

    if (!_hWnd) return;

    LONG lStyles = GetWindowLong(_hWnd, GWL_STYLE);
    if (lStyles & WS_MAXIMIZE) *isMaximized = true;
}

void Photino::SetMaximized(const bool maximized)
{
    assert(_hWnd);
    if (!_hWnd) return;

    ShowWindow(_hWnd, maximized ? SW_MAXIMIZE : SW_RESTORE);
}

void Photino::GetMinimized(bool* isMinimized) const
{
    assert(isMinimized && _hWnd);
    if (!isMinimized) return;

    *isMinimized = false;

    if (!_hWnd) return;

    LONG lStyles = GetWindowLong(_hWnd, GWL_STYLE);
    if (lStyles & WS_MINIMIZE) *isMinimized = true;
}

void Photino::SetMinimized(const bool minimized)
{
    assert(_hWnd);
    if (!_hWnd) return;

    ShowWindow(_hWnd, minimized ? SW_MINIMIZE : SW_RESTORE);
}

void Photino::GetResizable(bool* resizable) const
{
    assert(resizable && _hWnd);
    if (!resizable) return;

    *resizable = false;

    if (!_hWnd) return;

    LONG lStyles = GetWindowLong(_hWnd, GWL_STYLE);
    if (lStyles & WS_THICKFRAME) *resizable = true;
}

void Photino::SetResizable(const bool resizable)
{
    assert(_hWnd);
    if (!_hWnd) return;

    LONG_PTR style = GetWindowLongPtrW(_hWnd, GWL_STYLE);

    if (resizable)
        style |= (WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX);
    else
        style &= ~(WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX);

    SetLastError(0);
    LONG_PTR previousStyle = SetWindowLongPtrW(_hWnd, GWL_STYLE, style);
    assert(previousStyle != 0 || GetLastError() == ERROR_SUCCESS);
    // force non-client recalculation
    BOOL result = SetWindowPos(_hWnd, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
    assert(result);
}

void Photino::GetTopmost(bool* topmost) const
{
    assert(topmost);
    if (!topmost) return;

    *topmost = false;

    if (!_hWnd) return;

    LONG lStyles = GetWindowLong(_hWnd, GWL_EXSTYLE);
    if (lStyles & WS_EX_TOPMOST) *topmost = true;
}

void Photino::SetTopmost(const bool topmost)
{
    assert(_hWnd);
    if (!_hWnd) return;

    BOOL result = SetWindowPos(
        _hWnd,
        topmost ? HWND_TOPMOST : HWND_NOTOPMOST,
        0,
        0,
        0,
        0,
        SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

    assert(result);
}