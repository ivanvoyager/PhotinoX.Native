#include "Photino.h"
#include "Photino.Callbacks.h"
#include "Photino.Strings.h"
#include "Photino.Windows.State.h"

#include "Dependencies/wintoastlib.h"
#include <WinUser.h>
#include <Shellscalingapi.h>

#include <cassert>

#pragma comment(lib, "Shcore.lib")//TODO remove

using namespace WinToastLib;
using namespace PhotinoX::Native;

HWND Photino::GetHwnd() const noexcept { return platform_->hWnd; }

void Photino::Close() const
{
    assert(platform_->hWnd);
    if (!platform_->hWnd) return;

    BOOL result = PostMessageW(platform_->hWnd, WM_CLOSE, 0, 0);
    assert(result);
}

void Photino::SetTitle(const PlatformString& title)
{
    assert(platform_->hWnd);
    if (!platform_->hWnd) return;

    if (!SetWindowTextW(platform_->hWnd, title.c_str()))
        return;

    windowTitle_ = title;

    if (notificationsEnabled_)
    {
        WinToast::instance()->setAppName(title);

        if (notificationRegistrationId_.empty())
            WinToast::instance()->setAppUserModelId(title);
    }
}


void Photino::SetIconFile(const PlatformString& filename)
{
    assert(platform_->hWnd);
    if (!platform_->hWnd || filename.empty())
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

    iconFileName_ = filename;

    if (iconSmall)
        SendMessageW(platform_->hWnd, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(iconSmall));

    if (iconBig)
        SendMessageW(platform_->hWnd, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(iconBig));
}

void Photino::GetPosition(int* x, int* y) const
{
    assert((x || y) && platform_->hWnd);

    if (!x && !y) return;

    if (x) *x = 0;
    if (y) *y = 0;

    if (!platform_->hWnd) return;

    RECT rect{};
    if (!GetWindowRect(platform_->hWnd, &rect)) return;

    if (x) *x = rect.left;
    if (y) *y = rect.top;
}

void Photino::SetPosition(const int x, const int y)
{
    assert(platform_->hWnd);
    if (!platform_->hWnd) return;

    BOOL result = SetWindowPos(platform_->hWnd, nullptr, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
    assert(result);
}

void Photino::GetSize(int* width, int* height) const
{
    assert((width || height) && platform_->hWnd);
    if (!width && !height) return;

    if (width) *width = 0;
    if (height) *height = 0;

    if (!platform_->hWnd) return;

    RECT rect{};
    if (!GetWindowRect(platform_->hWnd, &rect)) return;

    if (width) *width = rect.right - rect.left;
    if (height) *height = rect.bottom - rect.top;
}

void Photino::SetSize(const int width, const int height)
{
    assert(platform_->hWnd);
    if (!platform_->hWnd) return;

    if (width <= 0 || height <= 0)
        return;

    int newWidth = width;
    int newHeight = height;

    if (platform_->sizeLimits.minWidth > 0 && newWidth < platform_->sizeLimits.minWidth)    newWidth = platform_->sizeLimits.minWidth;
    if (platform_->sizeLimits.minHeight > 0 && newHeight < platform_->sizeLimits.minHeight) newHeight = platform_->sizeLimits.minHeight;
    if (platform_->sizeLimits.maxWidth > 0 && newWidth > platform_->sizeLimits.maxWidth)    newWidth = platform_->sizeLimits.maxWidth;
    if (platform_->sizeLimits.maxHeight > 0 && newHeight > platform_->sizeLimits.maxHeight) newHeight = platform_->sizeLimits.maxHeight;

    BOOL result = SetWindowPos(platform_->hWnd, nullptr, 0, 0, newWidth, newHeight, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
    assert(result);
}

void Photino::SetMinSize(const int width, const int height)
{
    platform_->sizeLimits.minWidth = (std::max)(0, width);
    platform_->sizeLimits.minHeight = (std::max)(0, height);

    if (platform_->sizeLimits.maxWidth > 0 && platform_->sizeLimits.minWidth > platform_->sizeLimits.maxWidth)
        platform_->sizeLimits.maxWidth = platform_->sizeLimits.minWidth;
    if (platform_->sizeLimits.maxHeight > 0 && platform_->sizeLimits.minHeight > platform_->sizeLimits.maxHeight)
        platform_->sizeLimits.maxHeight = platform_->sizeLimits.minHeight;

    assert(platform_->hWnd);
    if (!platform_->hWnd) return;

    int currWidth = 0, currHeight = 0;
    GetSize(&currWidth, &currHeight);

    int newWidth = currWidth < platform_->sizeLimits.minWidth ? platform_->sizeLimits.minWidth : currWidth;
    int newHeight = currHeight < platform_->sizeLimits.minHeight ? platform_->sizeLimits.minHeight : currHeight;

    if (newWidth != currWidth || newHeight != currHeight)
        SetSize(newWidth, newHeight);
}

void Photino::SetMaxSize(const int width, const int height)
{
    platform_->sizeLimits.maxWidth = (std::max)(0, width);
    platform_->sizeLimits.maxHeight = (std::max)(0, height);

    if (platform_->sizeLimits.maxWidth > 0 && platform_->sizeLimits.maxWidth < platform_->sizeLimits.minWidth)
        platform_->sizeLimits.minWidth = platform_->sizeLimits.maxWidth;

    if (platform_->sizeLimits.maxHeight > 0 && platform_->sizeLimits.maxHeight < platform_->sizeLimits.minHeight)
        platform_->sizeLimits.minHeight = platform_->sizeLimits.maxHeight;

    assert(platform_->hWnd);
    if (!platform_->hWnd) return;

    int currWidth = 0, currHeight = 0;
    GetSize(&currWidth, &currHeight);

    int newWidth = platform_->sizeLimits.maxWidth > 0 && currWidth > platform_->sizeLimits.maxWidth ? platform_->sizeLimits.maxWidth : currWidth;
    int newHeight = platform_->sizeLimits.maxHeight > 0 && currHeight > platform_->sizeLimits.maxHeight ? platform_->sizeLimits.maxHeight : currHeight;

    if (newWidth != currWidth || newHeight != currHeight)
        SetSize(newWidth, newHeight);
}

void Photino::Center() const
{
    assert(platform_->hWnd);
    if (!platform_->hWnd) return;

    HMONITOR monitor = MonitorFromWindow(platform_->hWnd, MONITOR_DEFAULTTONEAREST);
    MONITORINFO monitorInfo{sizeof(monitorInfo)};
    if (!GetMonitorInfoW(monitor, &monitorInfo)) return;

    RECT windowRect{};
    if (!GetWindowRect(platform_->hWnd, &windowRect)) return;

    int windowWidth = windowRect.right - windowRect.left;
    int windowHeight = windowRect.bottom - windowRect.top;

    const RECT& work = monitorInfo.rcWork;

    int left = work.left + (work.right - work.left - windowWidth) / 2;
    int top = work.top + (work.bottom - work.top - windowHeight) / 2;

    BOOL result = SetWindowPos(platform_->hWnd, nullptr, left, top, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
    assert(result);
}

void Photino::Restore() const
{
    assert(platform_->hWnd);
    if (!platform_->hWnd) return;

    ShowWindow(platform_->hWnd, SW_RESTORE);
}

unsigned int Photino::GetScreenDpi() const
{
    assert(platform_->hWnd);
    if (!platform_->hWnd) return 96;

    UINT dpi = GetDpiForWindow(platform_->hWnd);
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

    *fullScreen = fullScreen_;
}

void Photino::SetFullScreen(const bool fullScreen)
{
    assert(platform_->hWnd);
    if (!platform_->hWnd) return;

    fullScreen_ = fullScreen;
    LONG_PTR style = GetWindowLongPtrW(platform_->hWnd, GWL_STYLE);
    if (fullScreen)
    {
        style |= WS_POPUP;
        style &= ~WS_OVERLAPPEDWINDOW;
    }
    else
    {
        if (chromeless_)
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
    LONG_PTR previousStyle = SetWindowLongPtrW(platform_->hWnd, GWL_STYLE, style);
    assert(previousStyle != 0 || GetLastError() == ERROR_SUCCESS);

    HMONITOR monitor = MonitorFromWindow(platform_->hWnd, MONITOR_DEFAULTTONEAREST);
    MONITORINFO monitorInfo{sizeof(monitorInfo)};

    if (GetMonitorInfoW(monitor, &monitorInfo))
    {
        const RECT& rc = fullScreen
                             ? monitorInfo.rcMonitor
                             : monitorInfo.rcWork;

        HRESULT hr = SetWindowPos(platform_->hWnd, HWND_TOP, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, SWP_FRAMECHANGED);
        assert(SUCCEEDED(hr));
    }
    else
    {
        HRESULT hr = SetWindowPos(platform_->hWnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED);
        assert(SUCCEEDED(hr));
    }
}

void Photino::GetMaximized(bool* isMaximized) const
{
    assert(isMaximized && platform_->hWnd);
    if (!isMaximized) return;

    *isMaximized = false;

    if (!platform_->hWnd) return;

    LONG lStyles = GetWindowLong(platform_->hWnd, GWL_STYLE);
    if (lStyles & WS_MAXIMIZE) *isMaximized = true;
}

void Photino::SetMaximized(const bool maximized)
{
    assert(platform_->hWnd);
    if (!platform_->hWnd) return;

    ShowWindow(platform_->hWnd, maximized ? SW_MAXIMIZE : SW_RESTORE);
}

void Photino::GetMinimized(bool* isMinimized) const
{
    assert(isMinimized && platform_->hWnd);
    if (!isMinimized) return;

    *isMinimized = false;

    if (!platform_->hWnd) return;

    LONG lStyles = GetWindowLong(platform_->hWnd, GWL_STYLE);
    if (lStyles & WS_MINIMIZE) *isMinimized = true;
}

void Photino::SetMinimized(const bool minimized)
{
    assert(platform_->hWnd);
    if (!platform_->hWnd) return;

    ShowWindow(platform_->hWnd, minimized ? SW_MINIMIZE : SW_RESTORE);
}

void Photino::GetResizable(bool* resizable) const
{
    assert(resizable && platform_->hWnd);
    if (!resizable) return;

    *resizable = false;

    if (!platform_->hWnd) return;

    LONG lStyles = GetWindowLong(platform_->hWnd, GWL_STYLE);
    if (lStyles & WS_THICKFRAME) *resizable = true;
}

void Photino::SetResizable(const bool resizable)
{
    assert(platform_->hWnd);
    if (!platform_->hWnd) return;

    LONG_PTR style = GetWindowLongPtrW(platform_->hWnd, GWL_STYLE);

    if (resizable)
        style |= (WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX);
    else
        style &= ~(WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX);

    SetLastError(0);
    LONG_PTR previousStyle = SetWindowLongPtrW(platform_->hWnd, GWL_STYLE, style);
    assert(previousStyle != 0 || GetLastError() == ERROR_SUCCESS);
    // force non-client recalculation
    BOOL result = SetWindowPos(platform_->hWnd, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
    assert(result);
}

void Photino::GetTopmost(bool* topmost) const
{
    assert(topmost);
    if (!topmost) return;

    *topmost = false;

    if (!platform_->hWnd) return;

    LONG lStyles = GetWindowLong(platform_->hWnd, GWL_EXSTYLE);
    if (lStyles & WS_EX_TOPMOST) *topmost = true;
}

void Photino::SetTopmost(const bool topmost)
{
    assert(platform_->hWnd);
    if (!platform_->hWnd) return;

    BOOL result = SetWindowPos(
        platform_->hWnd,
        topmost ? HWND_TOPMOST : HWND_NOTOPMOST,
        0,
        0,
        0,
        0,
        SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

    assert(result);
}