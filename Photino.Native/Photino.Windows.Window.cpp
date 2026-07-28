#include "Photino.h"
#include "Photino.Enums.h"
#include "Photino.Callbacks.h"
#include "Photino.Strings.h"
#include "Photino.Windows.State.h"
#include "Photino.Windows.Debug.h"

#include "Dependencies/wintoastlib.h"
#include <WinUser.h>
#include <Shellscalingapi.h>

#include <algorithm>
#include <cassert>

#pragma comment(lib, "Shcore.lib")//TODO remove

using namespace WinToastLib;
using namespace PhotinoX::Native;

namespace 
{
    bool TryGetMonitorInfo(const HMONITOR hMonitor, Monitor& monitor) noexcept
    {
        if (!hMonitor)
            return false;

        MONITORINFO info{};
        info.cbSize = sizeof(info);

        if (!GetMonitorInfoW(hMonitor, &info))
            return false;

        UINT dpiX = 96;
        UINT dpiY = 96;

        if (FAILED(GetDpiForMonitor(hMonitor, MDT_EFFECTIVE_DPI, &dpiX, &dpiY)))
            dpiX = dpiY = 96;

        monitor.monitor.x = info.rcMonitor.left;
        monitor.monitor.y = info.rcMonitor.top;
        monitor.monitor.width = info.rcMonitor.right - info.rcMonitor.left;
        monitor.monitor.height = info.rcMonitor.bottom - info.rcMonitor.top;

        monitor.work.x = info.rcWork.left;
        monitor.work.y = info.rcWork.top;
        monitor.work.width = info.rcWork.right - info.rcWork.left;
        monitor.work.height = info.rcWork.bottom - info.rcWork.top;

        monitor.scale = static_cast<double>(dpiY) / 96.0;

        return true;
    }

    struct MonitorEnumState
    {
        GetAllMonitorsCallback callback;
        void* state;
        bool stopped = false;
    };

    // https://learn.microsoft.com/en-us/windows/win32/api/winuser/nc-winuser-monitorenumproc
    // To continue the enumeration, return TRUE.
    // To stop the enumeration, return FALSE.
    BOOL MonitorEnum(const HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, const LPARAM dwData)
    {
        auto enumState = reinterpret_cast<MonitorEnumState*>(dwData);
        if (!enumState || !enumState->callback)
            return FALSE;

        Monitor props{};
        if (!TryGetMonitorInfo(hMonitor, props))
            return TRUE;

        if (!enumState->callback(&props, enumState->state))
        {
            enumState->stopped = true;
            return FALSE;
        }

        return TRUE;
    }
}

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

    options_.windowTitle = title;

    if (options_.notificationsEnabled)
    {
        WinToast::instance()->setAppName(options_.windowTitle);

        if (options_.notificationRegistrationId.empty())
            WinToast::instance()->setAppUserModelId(options_.windowTitle);
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

    options_.iconFileName = filename;

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

bool Photino::Activate() const
{
    assert(platform_->hWnd);
    if (!platform_->hWnd) return false;

    return SetForegroundWindow(platform_->hWnd) != FALSE;
}

bool Photino::Center() const
{
    assert(platform_->hWnd);
    if (!platform_->hWnd) return false;

    HMONITOR monitor = MonitorFromWindow(platform_->hWnd, MONITOR_DEFAULTTONEAREST);
    MONITORINFO monitorInfo{sizeof(monitorInfo)};
    if (!GetMonitorInfoW(monitor, &monitorInfo))
        return false;

    RECT windowRect{};
    if (!GetWindowRect(platform_->hWnd, &windowRect))
        return false;

    int windowWidth = windowRect.right - windowRect.left;
    int windowHeight = windowRect.bottom - windowRect.top;

    const RECT& work = monitorInfo.rcWork;

    int left = work.left + (work.right - work.left - windowWidth) / 2;
    int top = work.top + (work.bottom - work.top - windowHeight) / 2;

    BOOL result = SetWindowPos(
        platform_->hWnd,
        nullptr,
        left,
        top,
        0,
        0,
        SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);

    assert(result);
    return result != FALSE;
}

bool Photino::Maximize()
{
    PHOTINO_WINDOWS_LOG("[windows-command] Maximize\n");

    assert(platform_->hWnd);
    if (!platform_->hWnd) return false;

    return ShowWindowAfterFullScreenExit(SW_MAXIMIZE);
}

bool Photino::Minimize()
{
    PHOTINO_WINDOWS_LOG("[windows-command] Minimize\n");

    assert(platform_->hWnd);
    if (!platform_->hWnd) return false;

    return ShowWindowAfterFullScreenExit(SW_MINIMIZE);
}

bool Photino::Restore()
{
    PHOTINO_WINDOWS_LOG("[windows-command] Restore\n");

    assert(platform_->hWnd);
    if (!platform_->hWnd) return false;

    if (GetPlatformWindowState() == PhotinoWindowState::FullScreen ||
        platform_->hasFullScreenRestoreState)
    {
        SetFullScreen(false);

        return GetPlatformWindowState() != PhotinoWindowState::FullScreen &&
               !platform_->hasFullScreenRestoreState;
    }

    ShowWindow(platform_->hWnd, SW_RESTORE);
    UpdateWindowState();

    return true;
}

bool Photino::Show() const
{
    assert(platform_->hWnd);
    if (!platform_->hWnd) return false;

    if (!platform_->isAlreadyShown)
    {
        ShowWindow(platform_->hWnd, platform_->initialShowCommand);
        platform_->isAlreadyShown = true;

        UpdateWindow(platform_->hWnd);
    }
    else
    {
        ShowWindow(platform_->hWnd, SW_SHOW);
    }

    return true;
}

void Photino::BeginWindowDrag() const
{
    assert(platform_->hWnd);
    if (!platform_->hWnd) return;

    // Hand the drag off to the OS's non-client move loop so the window tracks
    // the cursor just like a native title bar would. Releasing capture first
    // lets the top-level window take the mouse over from the WebView2 control.
    POINT cursor{};
    GetCursorPos(&cursor);
    ReleaseCapture();
    SendMessageW(platform_->hWnd, WM_NCLBUTTONDOWN, HTCAPTION, MAKELPARAM(cursor.x, cursor.y));
}

void Photino::BeginWindowResize(const PhotinoWindowEdge edge) const
{
    assert(platform_->hWnd);
    if (!platform_->hWnd) return;

    WPARAM hitTest;
    switch (edge)
    {
    case PhotinoWindowEdge::Top:         hitTest = HTTOP;         break;
    case PhotinoWindowEdge::Bottom:      hitTest = HTBOTTOM;      break;
    case PhotinoWindowEdge::Left:        hitTest = HTLEFT;        break;
    case PhotinoWindowEdge::Right:       hitTest = HTRIGHT;       break;
    case PhotinoWindowEdge::TopLeft:     hitTest = HTTOPLEFT;     break;
    case PhotinoWindowEdge::TopRight:    hitTest = HTTOPRIGHT;    break;
    case PhotinoWindowEdge::BottomLeft:  hitTest = HTBOTTOMLEFT;  break;
    case PhotinoWindowEdge::BottomRight: hitTest = HTBOTTOMRIGHT; break;
    default: return;
    }

    // Same non-client hand-off as the drag, but starting a resize loop on the
    // grabbed edge or corner instead of a move.
    POINT cursor{};
    GetCursorPos(&cursor);
    ReleaseCapture();
    SendMessageW(platform_->hWnd, WM_NCLBUTTONDOWN, hitTest, MAKELPARAM(cursor.x, cursor.y));
}

unsigned int Photino::GetScreenDpi() const
{
    assert(platform_->hWnd);
    if (!platform_->hWnd) return 96;

    UINT dpi = GetDpiForWindow(platform_->hWnd);
    return dpi ? dpi : 96;
}

bool Photino::GetAllMonitors(GetAllMonitorsCallback callback, void* state) const noexcept
{
    assert(callback);
    if (!callback) return false;

    MonitorEnumState enumState{callback, state};

    BOOL result = EnumDisplayMonitors(nullptr, nullptr, MonitorEnum, reinterpret_cast<LPARAM>(&enumState));
    return result != FALSE || enumState.stopped;
}

bool Photino::GetWindowMonitor(Monitor& monitor) const noexcept
{
    assert(platform_->hWnd);
    if (!platform_->hWnd)
        return false;

    HMONITOR hMonitor = MonitorFromWindow(platform_->hWnd, MONITOR_DEFAULTTONEAREST);
    if (!hMonitor) return false;

    return TryGetMonitorInfo(hMonitor, monitor);
}

bool Photino::SaveFullScreenRestoreState()
{
    if (!platform_->hWnd)
        return false;

    if (platform_->hasFullScreenRestoreState)
        return true;

    SetLastError(0);
    const LONG_PTR style = GetWindowLongPtrW(platform_->hWnd, GWL_STYLE);
    const DWORD lastError = GetLastError();

    assert(style != 0 || lastError == ERROR_SUCCESS);

    if (style == 0 && lastError != ERROR_SUCCESS)
        return false;

    platform_->fullScreenPlacement.length = sizeof(platform_->fullScreenPlacement);

    if (GetWindowPlacement(platform_->hWnd, &platform_->fullScreenPlacement) == FALSE)
    {
        platform_->fullScreenStyle = 0;
        platform_->hasFullScreenRestoreState = false;
        return false;
    }

    platform_->fullScreenStyle = style;
    platform_->hasFullScreenRestoreState = true;

    return true;
}

bool Photino::RestoreFullScreenRestoreState()
{
    if (!platform_->hWnd)
        return false;

    if (platform_->fullScreenStyle != 0)
    {
        SetLastError(0);
        LONG_PTR previousStyle = SetWindowLongPtrW(platform_->hWnd, GWL_STYLE, platform_->fullScreenStyle);
        const DWORD lastError = GetLastError();
        assert(previousStyle != 0 || lastError == ERROR_SUCCESS);

        if (previousStyle == 0 && lastError != ERROR_SUCCESS)
            return false;
    }

    if (platform_->hasFullScreenRestoreState)
    {
        BOOL placementResult = SetWindowPlacement(platform_->hWnd, &platform_->fullScreenPlacement);
        assert(placementResult);

        if (!placementResult)
            return false;
    }

    return true;
}

void Photino::ResetFullScreenRestoreState()
{
    platform_->fullScreenStyle = 0;
    platform_->hasFullScreenRestoreState = false;
}

bool Photino::EnterFullScreen()
{
    if (!platform_->hWnd)
        return false;

    HMONITOR monitor = MonitorFromWindow(platform_->hWnd, MONITOR_DEFAULTTONEAREST);
    if (!monitor)
        return false;

    MONITORINFO monitorInfo{sizeof(monitorInfo)};
    if (!GetMonitorInfoW(monitor, &monitorInfo))
        return false;

    LONG_PTR style = platform_->fullScreenStyle;
    style |= WS_POPUP;
    style &= ~WS_OVERLAPPEDWINDOW;

    SetLastError(0);
    LONG_PTR previousStyle = SetWindowLongPtrW(platform_->hWnd, GWL_STYLE, style);
    const DWORD lastError = GetLastError();
    assert(previousStyle != 0 || lastError == ERROR_SUCCESS);

    if (previousStyle == 0 && lastError != ERROR_SUCCESS)
        return false;

    const RECT& rc = monitorInfo.rcMonitor;

    BOOL result = SetWindowPos(
        platform_->hWnd,
        HWND_TOP,
        rc.left,
        rc.top,
        rc.right - rc.left,
        rc.bottom - rc.top,
        SWP_NOOWNERZORDER | SWP_FRAMECHANGED);

    assert(result);
    return result != FALSE;
}

bool Photino::ExitFullScreen()
{
    if (!platform_->hWnd)
        return false;

    BOOL result = SetWindowPos(
        platform_->hWnd,
        nullptr,
        0,
        0,
        0,
        0,
        SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);

    assert(result);

    return result != FALSE;
}

bool Photino::ShowWindowAfterFullScreenExit(const int showCommand)
{
    PHOTINO_WINDOWS_LOG("[windows-command] ShowWindowAfterFullScreenExit(%d)\n", showCommand);

    const bool wasFullScreen =
        GetPlatformWindowState() == PhotinoWindowState::FullScreen ||
        platform_->hasFullScreenRestoreState;

    if (!wasFullScreen)
    {
        ShowWindow(platform_->hWnd, showCommand);
        UpdateWindowState();
        return true;
    }

    const bool previousSuppressWindowStateCallbacks = suppressWindowStateCallbacks_;

    suppressWindowStateCallbacks_ = true;

    SetFullScreen(false);

    if (GetPlatformWindowState() == PhotinoWindowState::FullScreen ||
        platform_->hasFullScreenRestoreState)
    {
        suppressWindowStateCallbacks_ = previousSuppressWindowStateCallbacks;
        return false;
    }

    ShowWindow(platform_->hWnd, showCommand);

    suppressWindowStateCallbacks_ = previousSuppressWindowStateCallbacks;

    options_.windowState = PhotinoWindowState::FullScreen;
    UpdateWindowState();

    return true;
}

bool Photino::IsFullScreen() const noexcept
{
    if (!platform_->hWnd)
        return false;

    RECT windowRect{};
    if (!GetWindowRect(platform_->hWnd, &windowRect))
        return false;

    HMONITOR monitor = MonitorFromWindow(platform_->hWnd, MONITOR_DEFAULTTONEAREST);
    if (!monitor)
        return false;

    MONITORINFO monitorInfo{sizeof(monitorInfo)};
    if (!GetMonitorInfoW(monitor, &monitorInfo))
        return false;

    return EqualRect(&windowRect, &monitorInfo.rcMonitor) != FALSE;
}

bool Photino::IsMinimized() const noexcept
{
    if (!platform_->hWnd)
        return false;

    return IsIconic(platform_->hWnd);// Determines whether the specified window is minimized (iconic).
}

bool Photino::IsMaximized() const noexcept
{
    if (!platform_->hWnd)
        return false;

    return IsZoomed(platform_->hWnd); // Determines whether a window is maximized.
}

PhotinoWindowState Photino::GetPlatformWindowState() const noexcept
{
    if (!platform_->hWnd)
        return options_.windowState;

    if (IsFullScreen())
        return PhotinoWindowState::FullScreen;

    if (IsMinimized())
        return PhotinoWindowState::Minimized;

    if (IsMaximized())
        return PhotinoWindowState::Maximized;

    return PhotinoWindowState::Normal;
}

bool Photino::SkipFullScreenChange(const bool fullScreen) const noexcept
{
    return fullScreen
               ? IsFullScreen()
               : !IsFullScreen() && !platform_->hasFullScreenRestoreState;
}

void Photino::SetFullScreen(const bool fullScreen)
{
    PHOTINO_WINDOWS_LOG("[windows-command] SetFullScreen(%d)\n", fullScreen);

    assert(platform_->hWnd);
    if (!platform_->hWnd) return;

    if (SkipFullScreenChange(fullScreen))
        return;

    const auto previousWindowState = options_.windowState;
    const bool previousSuppressWindowStateCallbacks = suppressWindowStateCallbacks_;

    suppressWindowStateCallbacks_ = true;

    bool success = false;

    if (fullScreen)
    {
        if (IsMinimized())
            ShowWindow(platform_->hWnd, SW_RESTORE);

        if (SaveFullScreenRestoreState() && EnterFullScreen())
        {
            success = true;
        }
        else
        {
            const bool restored = RestoreFullScreenRestoreState();
            const bool exited = ExitFullScreen();

            if (restored && exited)
                ResetFullScreenRestoreState();
        }
    }
    else
    {
        if (RestoreFullScreenRestoreState() && ExitFullScreen())
        {
            ResetFullScreenRestoreState();
            success = true;
        }
    }

    if (!success)
    {
        suppressWindowStateCallbacks_ = previousSuppressWindowStateCallbacks;
        return;
    }

    if (previousSuppressWindowStateCallbacks)
    {
        UpdateWindowState();
        suppressWindowStateCallbacks_ = previousSuppressWindowStateCallbacks;
        return;
    }

    options_.windowState = previousWindowState;
    suppressWindowStateCallbacks_ = false;
    UpdateWindowState();
}

void Photino::SetMaximized(const bool maximized)
{
    assert(platform_->hWnd);
    if (!platform_->hWnd) return;

    if (maximized)
    {
        Maximize();
        return;
    }

    if (IsFullScreen())
    {
        UpdateWindowState();
        return;
    }

    if (IsMaximized())
        ShowWindow(platform_->hWnd, SW_RESTORE);

    UpdateWindowState();
}

void Photino::SetMinimized(const bool minimized)
{
    assert(platform_->hWnd);
    if (!platform_->hWnd) return;

    if (minimized)
    {
        Minimize();
        return;
    }

    if (IsFullScreen())
    {
        UpdateWindowState();
        return;
    }

    if (IsMinimized())
        ShowWindow(platform_->hWnd, SW_RESTORE);

    UpdateWindowState();
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