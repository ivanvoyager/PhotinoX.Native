#include "Photino.h"
#include "Photino.Dialog.h"
#include "Photino.Memory.h"
#include "Photino.Windows.DarkMode.h"
#include "Photino.Windows.ToastHandler.h"

#include <algorithm>
#include <atomic>
#include <cassert>
#include <comdef.h>
#include <map>
#include <mutex>
#include <Shellscalingapi.h>
#include <Shlwapi.h>
#include <WebView2EnvironmentOptions.h>
#include <windows.h>
#include <wrl.h>

#pragma comment(lib, "Shcore.lib")
#pragma comment(lib, "Urlmon.lib")

#define WM_USER_INVOKE (WM_USER + 0x0002)

using namespace WinToastLib;
using namespace Microsoft::WRL;
using namespace PhotinoX::Native;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

namespace
{
    constexpr LPCWSTR CLASS_NAME = L"PhotinoX";
    PlatformString g_webview2RuntimePath;

    HINSTANCE g_hInstance = nullptr;
    std::atomic g_messageLoopRunning{ false };
    HWND g_uiThreadWindowHandle = nullptr;
    std::atomic g_isShuttingDown{ false };
    std::map<HWND, Photino*> g_hwndToPhotino;
}

const HBRUSH darkBrush = CreateSolidBrush(RGB(0, 0, 0));
const HBRUSH lightBrush = CreateSolidBrush(RGB(255, 255, 255));

void Photino::Register(const HINSTANCE hInstance)
{
    InitDarkModeSupport();

    g_hInstance = hInstance;

    // Register the window class
    WNDCLASSEX wcx;
    wcx.cbSize = sizeof(WNDCLASSEX);
    wcx.style = CS_HREDRAW | CS_VREDRAW;
    wcx.lpfnWndProc = WindowProc;
    wcx.cbClsExtra = 0;
    wcx.cbWndExtra = 0;
    wcx.hInstance = hInstance;
    wcx.hIcon = LoadIcon(hInstance, IDI_APPLICATION);
    wcx.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcx.hbrBackground = IsDarkModeEnabled() ? darkBrush : lightBrush;
    wcx.lpszMenuName = nullptr;
    wcx.lpszClassName = CLASS_NAME;
    wcx.hIconSm = LoadIcon(hInstance, IDI_APPLICATION);

    if (!RegisterClassExW(&wcx))
    {
        if (GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
            std::abort();
    }

    DPI_AWARENESS_CONTEXT previous = SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    assert(previous != nullptr);
}

Photino::Photino(PhotinoInitParams* initParams)
{
    assert(initParams);
    if (!initParams)
        std::abort();

    //wchar_t msg[50];
    //swprintf(msg, 50, L"Size: %i", initParams->Size);
    //MessageBox(nullptr, msg, L"", MB_OK);

    //wchar_t msg[50];
    //swprintf(msg, 50, L"MaxWidth: %i", initParams->MaxWidth);
    //MessageBox(nullptr, msg, L"", MB_OK);

    if (initParams->Size != sizeof(PhotinoInitParams))
    {
        wchar_t msg[200];
        swprintf(msg, 200, L"Initial parameters passed are %i bytes, but expected %I64i bytes.", initParams->Size, sizeof(PhotinoInitParams));
        MessageBoxW(nullptr, msg, L"Native Initialization Failed", MB_OK);
        std::abort();
    }

    _startString = ToPlatformString(initParams->StartString);
    _startUrl = ToPlatformString(initParams->StartUrl);
    _windowTitle = ToPlatformString(initParams->Title);
    _temporaryFilesPath = ToPlatformString(initParams->TemporaryFilesPath);
    _userAgent = ToPlatformString(initParams->UserAgent);
    _browserControlInitParameters = ToPlatformString(initParams->BrowserControlInitParameters);
    _notificationRegistrationId = ToPlatformString(initParams->NotificationRegistrationId);

    for (auto& customSchemeName : initParams->CustomSchemeNames)
    {
        AddCustomSchemeName(customSchemeName);
    }

    _parent = initParams->ParentInstance;

    // these handlers are ALWAYS hooked up
    _closingCallback = initParams->ClosingHandler;
    _focusInCallback = initParams->FocusInHandler;
    _focusOutCallback = initParams->FocusOutHandler;
    _resizedCallback = initParams->ResizedHandler;
    _maximizedCallback = initParams->MaximizedHandler;
    _restoredCallback = initParams->RestoredHandler;
    _minimizedCallback = initParams->MinimizedHandler;
    _movedCallback = initParams->MovedHandler;
    _webMessageReceivedCallback = initParams->WebMessageReceivedHandler;
    _customSchemeCallback = initParams->CustomSchemeHandler;
    _closedCallback = initParams->ClosedHandler;

    _zoom = initParams->Zoom;

    _sizeLimits.minWidth = (std::max)(0, initParams->MinWidth);
    _sizeLimits.minHeight = (std::max)(0, initParams->MinHeight);
    _sizeLimits.maxWidth = (std::max)(0, initParams->MaxWidth);
    _sizeLimits.maxHeight = (std::max)(0, initParams->MaxHeight);

    if (_sizeLimits.maxWidth > 0 && _sizeLimits.minWidth > _sizeLimits.maxWidth)    _sizeLimits.maxWidth = _sizeLimits.minWidth;
    if (_sizeLimits.maxHeight > 0 && _sizeLimits.minHeight > _sizeLimits.maxHeight) _sizeLimits.maxHeight = _sizeLimits.minHeight;

    _chromeless = initParams->Chromeless;
    _fullScreen = initParams->FullScreen;
    _transparentEnabled = initParams->Transparent;
    _contextMenuEnabled = initParams->ContextMenuEnabled;
    _zoomEnabled = initParams->ZoomEnabled;
    _devToolsEnabled = initParams->DevToolsEnabled;
    _grantBrowserPermissions = initParams->GrantBrowserPermissions;
    _mediaAutoplayEnabled = initParams->MediaAutoplayEnabled;
    _fileSystemAccessEnabled = initParams->FileSystemAccessEnabled;
    _webSecurityEnabled = initParams->WebSecurityEnabled;
    _javascriptClipboardAccessEnabled = initParams->JavascriptClipboardAccessEnabled;
    _mediaStreamEnabled = initParams->MediaStreamEnabled;
    _smoothScrollingEnabled = initParams->SmoothScrollingEnabled;
    _ignoreCertificateErrorsEnabled = initParams->IgnoreCertificateErrorsEnabled;
    _notificationsEnabled = initParams->NotificationsEnabled;

    //wchar_t msg[50];
    //swprintf(msg, 50, L"Height: %i  Width: %i  Left: %d  Top: %d", initParams->Height, initParams->Width, initParams->Left, initParams->Top);
    //MessageBox(nullptr, msg, L"", MB_OK);

    if (initParams->UseOsDefaultSize)
    {
        initParams->Width = CW_USEDEFAULT;
        initParams->Height = CW_USEDEFAULT;
    }
    else
    {
        if (initParams->Width < 0) initParams->Width = CW_USEDEFAULT;
        if (initParams->Height < 0) initParams->Height = CW_USEDEFAULT;
    }

    if (initParams->UseOsDefaultLocation)
    {
        initParams->Left = CW_USEDEFAULT;
        initParams->Top = CW_USEDEFAULT;
    }

    if (initParams->FullScreen)
    {
        initParams->Left = 0;
        initParams->Top = 0;
        initParams->Width = GetSystemMetrics(SM_CXSCREEN);
        initParams->Height = GetSystemMetrics(SM_CYSCREEN);
    }

    if (initParams->Chromeless)
    {
        //CW_USEDEFAULT CAN NOT BE USED ON POPUP WINDOWS
        if (initParams->Left == CW_USEDEFAULT && initParams->Top == CW_USEDEFAULT) initParams->CenterOnInitialize = true;
        if (initParams->Left == CW_USEDEFAULT) initParams->Left = 0;
        if (initParams->Top == CW_USEDEFAULT) initParams->Top = 0;
        if (initParams->Height == CW_USEDEFAULT) initParams->Height = 600;
        if (initParams->Width == CW_USEDEFAULT) initParams->Width = 800;
    }

    if (initParams->Height > initParams->MaxHeight && initParams->MaxHeight > 0)    initParams->Height = initParams->MaxHeight;
    if (initParams->Height < initParams->MinHeight && initParams->MinHeight > 0)    initParams->Height = initParams->MinHeight;
    if (initParams->Width > initParams->MaxWidth && initParams->MaxWidth > 0)       initParams->Width = initParams->MaxWidth;
    if (initParams->Width < initParams->MinWidth && initParams->MinWidth > 0)       initParams->Width = initParams->MinWidth;

    HWND parentHwnd = _parent ? _parent->GetHwnd() : nullptr;

    //Create the window
    _hWnd = CreateWindowExW(
        initParams->Transparent ? WS_EX_LAYERED : 0, //WS_EX_OVERLAPPEDWINDOW, //An optional extended window style.
        CLASS_NAME,					//Window class
        _windowTitle.c_str(),		//Window text
        initParams->Chromeless || initParams->FullScreen ? WS_POPUP : WS_OVERLAPPEDWINDOW,	//Window style

        // Size and position
        initParams->Left, initParams->Top, initParams->Width, initParams->Height,

        parentHwnd, // Parent window handle
        nullptr,    //Menu
        g_hInstance,//Instance handle
        this        //Additional application data
    );

    if (!_hWnd)
        std::abort();

    g_hwndToPhotino[_hWnd] = this;

    SetIconFile(ToPlatformString(initParams->WindowIconFile));

    if (initParams->CenterOnInitialize)
        Center();

    if (initParams->Minimized)
        SetMinimized(true);

    if (initParams->Maximized)
        SetMaximized(true);

    SetResizable(initParams->Resizable);

    if (initParams->Topmost)
        SetTopmost(true);

    if (_notificationsEnabled)
    {
        WinToast::instance()->setAppName(_windowTitle);
        if (!_notificationRegistrationId.empty())
            WinToast::instance()->setAppUserModelId(_notificationRegistrationId);
        else
            WinToast::instance()->setAppUserModelId(_windowTitle);

        _toastHandler = new WinToastHandler(this);
        WinToast::instance()->initialize();
    }

    _dialog = new PhotinoDialog(this);

    _isAlreadyShown = initParams->Minimized || initParams->Maximized;
    Show();
}

Photino::~Photino()
{
    delete _dialog;
    _dialog = nullptr;

    delete _toastHandler;
    _toastHandler = nullptr;
}

HWND Photino::GetHwnd() const
{
    assert(_hWnd);
    return _hWnd;
}

void Photino::ApplySizeLimits(MINMAXINFO& info) const
{
    if (_sizeLimits.minWidth > 0)   info.ptMinTrackSize.x = _sizeLimits.minWidth;
    if (_sizeLimits.minHeight > 0)  info.ptMinTrackSize.y = _sizeLimits.minHeight;
    if (_sizeLimits.maxWidth > 0)   info.ptMaxTrackSize.x = _sizeLimits.maxWidth;
    if (_sizeLimits.maxHeight > 0)  info.ptMaxTrackSize.y = _sizeLimits.maxHeight;
}

LRESULT CALLBACK WindowProc(const HWND hwnd, const UINT uMsg, const WPARAM wParam, const LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_CREATE: {
        EnableDarkMode(hwnd, true);
        if (IsDarkModeEnabled())
            RefreshNonClientArea(hwnd);
        return 0;
    }

    case WM_DPICHANGED:
    {
        //UINT dpiX = LOWORD(wParam);
        //UINT dpiY = HIWORD(wParam);

        const auto newWindowRect = reinterpret_cast<const RECT*>(lParam);

        SetWindowPos(
            hwnd,
            nullptr,
            newWindowRect->left,
            newWindowRect->top,
            newWindowRect->right - newWindowRect->left,
            newWindowRect->bottom - newWindowRect->top,
            SWP_NOZORDER | SWP_NOACTIVATE
            );

        return 0;
    }

    case WM_SETTINGCHANGE:
    {
        if (IsColorSchemeChange(lParam))
            SendMessageW(hwnd, WM_THEMECHANGED, 0, 0);

        return 0;
    }

    case WM_THEMECHANGED:
    {
        EnableDarkMode(hwnd, IsDarkModeEnabled());
        RefreshNonClientArea(hwnd);
        InvalidateRect(hwnd, nullptr, TRUE);
        return 0;
    }

    case WM_PAINT:
    {
        PAINTSTRUCT ps{};
        if (const HDC hdc = BeginPaint(hwnd, &ps))
        {
            // Fill the background with the current theme color
            if (IsDarkModeEnabled())
            {
                FillRect(hdc, &ps.rcPaint, darkBrush);
                // SetTextColor(hdc, RGB(255,255,255));
            }
            else
            {
                FillRect(hdc, &ps.rcPaint, lightBrush);
                // SetTextColor(hdc, RGB(0, 0, 0));
            }

            // Draw some text
            // SetBkMode(hdc, TRANSPARENT);
            // TextOut(hdc, 10, 10, L"Hello, World! (Dynamic Theme)", 31);

            EndPaint(hwnd, &ps);
        }
        return 0;
    }

    case WM_ACTIVATE:
    {
        if (const auto it = g_hwndToPhotino.find(hwnd); it != g_hwndToPhotino.end())
        {
            const auto photino = it->second;

            if (LOWORD(wParam) == WA_INACTIVE)
            {
                photino->InvokeFocusOut();
            }
            else
            {
                photino->FocusWebView2();
                photino->InvokeFocusIn();
            }
        }

        return 0;
    }

    case WM_CLOSE:
    {
        if (const auto it = g_hwndToPhotino.find(hwnd); it != g_hwndToPhotino.end())
        {
            const auto photino = it->second;

            if (!g_isShuttingDown.load(std::memory_order_acquire))
            {
                if (photino && photino->InvokeClosing())
                    return 0;
            }

            DestroyWindow(hwnd);
            return 0;
        }

        break;
    }

    case WM_DESTROY:
    {
        if (const auto it = g_hwndToPhotino.find(hwnd); it != g_hwndToPhotino.end())
        {
            auto photino = it->second;
            g_hwndToPhotino.erase(it);

            if (photino)
            {
                photino->CloseWebView();
                photino->InvokeClose();
                delete photino;
            }
        }

        if (hwnd == g_uiThreadWindowHandle)
        {
            g_isShuttingDown.store(true, std::memory_order_release);
            g_uiThreadWindowHandle = nullptr;

            PostQuitMessage(0);
        }

        return 0;
    }

    case WM_USER_INVOKE:
    {
        auto callback = reinterpret_cast<InvokeCallback>(wParam);
        assert(callback);

        if (callback)
            callback();

        return 0;
    }

    case WM_GETMINMAXINFO:
    {
        if (const auto it = g_hwndToPhotino.find(hwnd); it != g_hwndToPhotino.end())
        {
            const auto photino = it->second;
            if (photino)
                photino->ApplySizeLimits(*reinterpret_cast<MINMAXINFO*>(lParam));
        }
        return 0;
    }

    case WM_SIZE:
    {
        if (const auto it = g_hwndToPhotino.find(hwnd); it != g_hwndToPhotino.end())
        {
            const auto photino = it->second;

            photino->RefitContent();

            int width = 0, height = 0;
            photino->GetSize(&width, &height);
            photino->InvokeResize(width, height);

            switch (wParam)
            {
            case SIZE_MAXIMIZED:
                photino->InvokeMaximized();
                break;
            case SIZE_RESTORED:
                photino->InvokeRestored();
                break;
            case SIZE_MINIMIZED:
                photino->InvokeMinimized();
                break;
            }
        }
        return 0;
    }

    case WM_MOVE:
    {
        if (const auto it = g_hwndToPhotino.find(hwnd); it != g_hwndToPhotino.end())
        {
            const auto photino = it->second;
            // photino->NotifyWebView2WindowMove();
            // photino->RefitContent();

            int x = 0, y = 0;
            photino->GetPosition(&x, &y);
            photino->InvokeMove(x, y);
        }
        return 0;
    }

    case WM_MOVING:
    {
        if (const auto it = g_hwndToPhotino.find(hwnd); it != g_hwndToPhotino.end())
        {
            const auto photino = it->second;

            // Photino->NotifyWebView2WindowMove();
            // Photino->RefitContent();
        }
        break;
    }

    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void Photino::CloseWebView()
{
    if (_webviewWindow != nullptr)
    {
        _webviewWindow->Stop();
        _webviewWindow = nullptr;
    }

    if (_webviewController != nullptr)
    {
        _webviewController->Close();
        _webviewController = nullptr;
    }

    if (_webviewEnvironment != nullptr)
    {
        _webviewEnvironment = nullptr;
    }

    _webViewInitialized = false;
    _scriptId.clear();
}

void Photino::Center()
{
    assert(_hWnd);
    if (!_hWnd)  return;

    HMONITOR monitor = MonitorFromWindow(_hWnd, MONITOR_DEFAULTTONEAREST);
    MONITORINFO monitorInfo{ sizeof(monitorInfo) };
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

void Photino::Close() const
{
    assert(_hWnd);
    if (!_hWnd) return;

    BOOL result = PostMessageW(_hWnd, WM_CLOSE, 0, 0);
    assert(result);
}

void Photino::GetTransparentEnabled(bool* enabled) const
{
    assert(enabled);
    if (!enabled) return;

    *enabled = _transparentEnabled;

    if (!_webviewController) return;

    wil::com_ptr<ICoreWebView2Controller2> controller2;
    if (FAILED(_webviewController->QueryInterface(&controller2)) || !controller2) return;

    COREWEBVIEW2_COLOR backgroundColor{};
    if (SUCCEEDED(controller2->get_DefaultBackgroundColor(&backgroundColor)))
        *enabled = (backgroundColor.A == 0);
}

void Photino::GetContextMenuEnabled(bool* enabled) const
{
    assert(enabled);
    if (!enabled) return;

    *enabled = _contextMenuEnabled;

    if (!_webviewWindow) return;

    wil::com_ptr<ICoreWebView2Settings> settings;
    if (FAILED(_webviewWindow->get_Settings(&settings)) || !settings) return;

    BOOL value = FALSE;
    if (SUCCEEDED(settings->get_AreDefaultContextMenusEnabled(&value)))
        *enabled = (value == TRUE);
}

void Photino::GetZoomEnabled(bool* enabled) const
{
    assert(enabled && _webviewWindow);
    if (!enabled) return;

    *enabled = _zoomEnabled;

    if (!_webviewWindow) return;

    wil::com_ptr<ICoreWebView2Settings> settings;
    if (FAILED(_webviewWindow->get_Settings(&settings)) || !settings) return;

    BOOL value = FALSE;
    if (SUCCEEDED(settings->get_IsZoomControlEnabled(&value)))
        *enabled = (value == TRUE);
}

void Photino::GetDevToolsEnabled(bool* enabled) const
{
    assert(enabled);
    if (!enabled) return;

    *enabled = _devToolsEnabled;

    if (!_webviewWindow) return;

    wil::com_ptr<ICoreWebView2Settings> settings;
    if (FAILED(_webviewWindow->get_Settings(&settings)) || !settings) return;

    BOOL value = FALSE;
    if (SUCCEEDED(settings->get_AreDevToolsEnabled(&value)))
        *enabled = (value == TRUE);
}

void Photino::GetFullScreen(bool* fullScreen) const
{
    assert(fullScreen);
    if (!fullScreen)
        return;

    *fullScreen = _fullScreen;
}

void Photino::GetGrantBrowserPermissions(bool* grant) const
{
    assert(grant);
    if (!grant) return;

    *grant = _grantBrowserPermissions;
}

void Photino::GetMediaAutoplayEnabled(bool* enabled) const
{
    assert(enabled);
    if (!enabled) return;

    *enabled = _mediaAutoplayEnabled;
}

void Photino::GetFileSystemAccessEnabled(bool* enabled) const
{
    assert(enabled);
    if (!enabled) return;

    *enabled = _fileSystemAccessEnabled;
}

void Photino::GetWebSecurityEnabled(bool* enabled) const
{
    assert(enabled);
    if (!enabled) return;

    *enabled = _webSecurityEnabled;
}

void Photino::GetJavascriptClipboardAccessEnabled(bool* enabled) const
{
    assert(enabled);
    if (!enabled) return;

    *enabled = _javascriptClipboardAccessEnabled;
}

void Photino::GetMediaStreamEnabled(bool* enabled) const
{
    assert(enabled);
    if (!enabled) return;

    *enabled = _mediaStreamEnabled;
}

void Photino::GetSmoothScrollingEnabled(bool* enabled) const
{
    assert(enabled);
    if (!enabled) return;

    *enabled = _smoothScrollingEnabled;
}

void Photino::GetIgnoreCertificateErrorsEnabled(bool* enabled) const
{
    assert(enabled);
    if (!enabled) return;

    *enabled = _ignoreCertificateErrorsEnabled;
}

void Photino::GetNotificationsEnabled(bool* enabled) const
{
    assert(enabled);
    if (!enabled) return;

    *enabled = _notificationsEnabled;
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

void Photino::GetMinimized(bool* isMinimized) const
{
    assert(isMinimized && _hWnd);
    if (!isMinimized)  return;

    *isMinimized = false;

    if (!_hWnd)  return;

    LONG lStyles = GetWindowLong(_hWnd, GWL_STYLE);
    if (lStyles & WS_MINIMIZE) *isMinimized = true;
}

void Photino::GetPosition(int* x, int* y) const
{
    assert((x || y) && _hWnd);

    if (!x && !y)  return;

    if (x) *x = 0;
    if (y) *y = 0;

    if (!_hWnd) return;

    RECT rect{};
    if (!GetWindowRect(_hWnd, &rect)) return;

    if (x) *x = rect.left;
    if (y) *y = rect.top;
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

unsigned int Photino::GetScreenDpi() const
{
    assert(_hWnd);
    if (!_hWnd) return 96;

    UINT dpi = GetDpiForWindow(_hWnd);
    return dpi ? dpi : 96;
}

void Photino::GetSize(int* width, int* height) const
{
    assert((width || height) && _hWnd);
    if (!width && !height) return;

    if (width)  *width = 0;
    if (height) *height = 0;

    if (!_hWnd) return;

    RECT rect{};
    if (!GetWindowRect(_hWnd, &rect)) return;

    if (width) *width = rect.right - rect.left;
    if (height) *height = rect.bottom - rect.top;
}

/*AutoString Photino::GetTitle() const
{
    //int titleLength = GetWindowTextLength(_hWnd) + 1;
    //wchar_t* title = new wchar_t[titleLength];
    //GetWindowText(_hWnd, title, titleLength);
    //MessageBox(nullptr, title, L"", MB_OK);
    return _windowTitle;
}
*/

void Photino::GetTopmost(bool* topmost) const
{
    assert(topmost);
    if (!topmost) return;

    *topmost = false;

    if (!_hWnd) return;

    LONG lStyles = GetWindowLong(_hWnd, GWL_EXSTYLE);
    if (lStyles & WS_EX_TOPMOST) *topmost = true;
}

void Photino::GetZoom(int* zoom) const
{
    assert(zoom);
    if (!zoom) return;

    *zoom = _zoom;

    if (!_webviewController) return;

    double rawValue = 0.0;
    if (FAILED(_webviewController->get_ZoomFactor(&rawValue))) return;

    rawValue = (rawValue * 100.0) + 0.5; // rounding
    *zoom = static_cast<int>(rawValue);
}

/*
 * The htmlContent parameter may not be larger than 2 MB (2 * 1024 * 1024 bytes) in total size.
 * The origin of the new page is about:blank.
 */
void Photino::NavigateToString(const PlatformString &content) const
{
    assert(_webviewWindow);
    if (!_webviewWindow) return;

    HRESULT hr = _webviewWindow->NavigateToString(content.c_str());
    assert(SUCCEEDED(hr));
}

void Photino::NavigateToUrl(const PlatformString &url) const
{
    assert(_webviewWindow);
    if (!_webviewWindow || url.empty()) return;

    HRESULT hr = _webviewWindow->Navigate(url.c_str());
    assert(SUCCEEDED(hr));
}

void Photino::Restore() const
{
    assert(_hWnd);
    if (!_hWnd) return;

    ShowWindow(_hWnd, SW_RESTORE);
}

void Photino::SendWebMessage(const PlatformString &message) const
{
    assert(_webviewWindow);
    if (!_webviewWindow) return;

    HRESULT hr = _webviewWindow->PostWebMessageAsString(message.c_str());
    assert(SUCCEEDED(hr));
}

void Photino::SetTransparentEnabled(const bool enabled)
{
    _transparentEnabled = enabled;

    assert(_webviewController && _webviewWindow);
    if (!_webviewController || !_webviewWindow) return;

    wil::com_ptr<ICoreWebView2Controller2> controller2;
    if (FAILED(_webviewController->QueryInterface(&controller2)) || !controller2) return;

    COREWEBVIEW2_COLOR backgroundColor{};
    HRESULT hr = controller2->get_DefaultBackgroundColor(&backgroundColor);
    if (SUCCEEDED(hr))
    {
        backgroundColor.A = enabled ? 0 : 255;
        hr = controller2->put_DefaultBackgroundColor(backgroundColor);
        assert(SUCCEEDED(hr));
    }

    hr = _webviewWindow->Reload();
    assert(SUCCEEDED(hr));
}

void Photino::SetContextMenuEnabled(const bool enabled)
{
    _contextMenuEnabled = enabled;
    if (!_webviewWindow) return;

    wil::com_ptr<ICoreWebView2Settings> settings;
    if (FAILED(_webviewWindow->get_Settings(&settings)) || !settings) return;

    HRESULT hr = settings->put_AreDefaultContextMenusEnabled(enabled ? TRUE : FALSE);
    assert(SUCCEEDED(hr));

    hr = _webviewWindow->Reload();
    assert(SUCCEEDED(hr));
}

void Photino::SetZoomEnabled(const bool enabled)
{
    _zoomEnabled = enabled;
    assert(_webviewWindow);
    if (!_webviewWindow) return;

    wil::com_ptr<ICoreWebView2Settings> settings;
    if (FAILED(_webviewWindow->get_Settings(&settings)) || !settings) return;

    HRESULT hr = settings->put_IsZoomControlEnabled(enabled ? TRUE : FALSE);
    assert(SUCCEEDED(hr));

    hr = _webviewWindow->Reload();
    assert(SUCCEEDED(hr));
}

void Photino::SetDevToolsEnabled(const bool enabled)
{
    _devToolsEnabled = enabled;

    if (!_webviewWindow) return;

    wil::com_ptr<ICoreWebView2Settings> settings;
    if (FAILED(_webviewWindow->get_Settings(&settings)) || !settings) return;

    HRESULT hr = settings->put_AreDevToolsEnabled(enabled ? TRUE : FALSE);
    if (FAILED(hr)) return;

    hr = _webviewWindow->Reload();
    assert(SUCCEEDED(hr));
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
    MONITORINFO monitorInfo{ sizeof(monitorInfo) };

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

void Photino::SetMinimized(const bool minimized)
{
    assert(_hWnd);
    if (!_hWnd) return;

    ShowWindow(_hWnd, minimized ? SW_MINIMIZE : SW_RESTORE);
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

void Photino::SetMaximized(const bool maximized)
{
    assert(_hWnd);
    if (!_hWnd) return;

    ShowWindow(_hWnd, maximized ? SW_MAXIMIZE : SW_RESTORE);
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

void Photino::SetPosition(const int x, const int y)
{
    assert(_hWnd);
    if (!_hWnd) return;

    BOOL result = SetWindowPos(_hWnd, nullptr, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
    assert(result);
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

void Photino::SetSize(const int width, const int height)
{
    assert(_hWnd);
    if (!_hWnd) return;

    if (width <= 0 || height <= 0)
        return;

    int newWidth = width;
    int newHeight = height;

    if (_sizeLimits.minWidth > 0 && newWidth < _sizeLimits.minWidth)
        newWidth = _sizeLimits.minWidth;

    if (_sizeLimits.minHeight > 0 && newHeight < _sizeLimits.minHeight)
        newHeight = _sizeLimits.minHeight;

    if (_sizeLimits.maxWidth > 0 && newWidth > _sizeLimits.maxWidth)
        newWidth = _sizeLimits.maxWidth;

    if (_sizeLimits.maxHeight > 0 && newHeight > _sizeLimits.maxHeight)
        newHeight = _sizeLimits.maxHeight;

    BOOL result = SetWindowPos(_hWnd, nullptr, 0, 0, newWidth, newHeight, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
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

void Photino::SetZoom(int zoom)
{
    if (zoom < 25)          zoom = 25;
    else if (zoom > 500)    zoom = 500;

    _zoom = zoom;

    assert(_webviewController);
    if (!_webviewController) return;

    double newZoom = static_cast<double>(zoom) / 100.0;
    HRESULT hr = _webviewController->put_ZoomFactor(newZoom);
    assert(SUCCEEDED(hr));
}

void Photino::ShowNotification(const PlatformString& title, const PlatformString& message) const
{
    if (!_notificationsEnabled || !_toastHandler || !WinToast::isCompatible())
        return;

    WinToastTemplate toast = WinToastTemplate(WinToastTemplate::ImageAndText02);
    toast.setTextField(title, WinToastTemplate::FirstLine);
    toast.setTextField(message, WinToastTemplate::SecondLine);

    if (!_iconFileName.empty())
        toast.setImagePath(_iconFileName);

    INT64 result = WinToast::instance()->showToast(toast, _toastHandler);
    assert(result >= 0);
}

void Photino::WaitForExit() const
{
    assert(_hWnd);
    if (!_hWnd) return;

    bool expected = false;
    if (!g_messageLoopRunning.compare_exchange_strong(expected, true, std::memory_order_acq_rel)) return;

    g_isShuttingDown.store(false, std::memory_order_release);
    g_uiThreadWindowHandle = _hWnd;

    // Run the message loop
    MSG msg{};
    while (true)
    {
        int result = GetMessageW(&msg, nullptr, 0, 0);
        if (result <= 0)        // 0 - WM_QUIT, -1 - error
            break;

        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    g_uiThreadWindowHandle = nullptr;
    g_isShuttingDown.store(true, std::memory_order_release);
    g_messageLoopRunning.store(false, std::memory_order_release);
}

//Callbacks
//https://learn.microsoft.com/en-us/windows/win32/api/winuser/nc-winuser-monitorenumproc
//To continue the enumeration, return TRUE.
//To stop the enumeration, return FALSE.
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

void Photino::GetAllMonitors(GetAllMonitorsCallback callback) const
{
    assert(callback);
    if (!callback) return;

    BOOL result = EnumDisplayMonitors(nullptr, nullptr, MonitorEnum, reinterpret_cast<LPARAM>(callback));
    assert(result);
}

void Photino::Invoke(InvokeCallback callback) const
{
    assert(_hWnd && callback);
    if (!_hWnd || !callback) return;

    if (g_isShuttingDown.load(std::memory_order_acquire)) return;

    DWORD windowThreadId = GetWindowThreadProcessId(_hWnd, nullptr);
    DWORD currentThreadId = GetCurrentThreadId();
    if (windowThreadId == currentThreadId)
    {
        callback();
        return;
    }

    if (!IsWindow(_hWnd))
        return;

    // Block until the callback is actually executed and completed
    LRESULT result = SendMessageW(_hWnd, WM_USER_INVOKE, reinterpret_cast<WPARAM>(callback), 0);
}

PlatformString Photino::BuildStartupString() const
{
    // TODO: Implement special startup strings.
    // https://peter.sh/experiments/chromium-command-line-switches/
    // https://learn.microsoft.com/en-us/dotnet/api/microsoft.web.webview2.core.corewebview2environmentoptions.additionalbrowserarguments?view=webview2-dotnet-1.0.1938.49&viewFallbackFrom=webview2-dotnet-1.0.1901.177view%3Dwebview2-1.0.1901.177
    // https://www.chromium.org/developers/how-tos/run-chromium-with-flags/
    // Add together all 7 special startup strings, plus the generic one passed by the user to make one big string. Try not to duplicate anything. Separate with spaces.

    PlatformString startupString;

    if (!_userAgent.empty())
    {
        PlatformString userAgent = _userAgent;
        std::ranges::replace(userAgent, L'"', L'\'');
        startupString += L"--user-agent=\"" + userAgent + L"\" ";
    }

    if (_mediaAutoplayEnabled)
        startupString += L"--autoplay-policy=no-user-gesture-required ";

    if (_fileSystemAccessEnabled)
        startupString += L"--allow-file-access-from-files ";

    if (!_webSecurityEnabled)
        startupString += L"--disable-web-security ";

    if (_javascriptClipboardAccessEnabled)
        startupString += L"--enable-javascript-clipboard-access ";

    if (_mediaStreamEnabled)
        startupString += L"--enable-usermedia-screen-capturing ";

    if (!_smoothScrollingEnabled)
        startupString += L"--disable-smooth-scrolling ";

    if (_ignoreCertificateErrorsEnabled)
        startupString += L"--ignore-certificate-errors ";

    if (!_browserControlInitParameters.empty())
    {
        if (!startupString.empty() && startupString.back() != L' ')
            startupString += L' ';
        startupString += _browserControlInitParameters; // e.g.--hide-scrollbars
    }

    return startupString;
}

HRESULT Photino::CompleteWebViewInitialization()
{
    assert(!_webViewInitialized);
    if (_webViewInitialized)
        return S_OK;

    _webViewInitialized = true;

    if (!_startUrl.empty())
    {
        NavigateToUrl(_startUrl);
    }
    else if (!_startString.empty())
    {
        NavigateToString(_startString);
    }
    else
    {
        MessageBoxW(nullptr, L"Neither StartUrl nor StartString was specified", L"Native Initialization Failed", MB_OK);
        std::abort();
    }

    if (_contextMenuEnabled == false)
        SetContextMenuEnabled(false);

    if (_zoomEnabled == false)
        SetZoomEnabled(false);

    if (_devToolsEnabled == false)
        SetDevToolsEnabled(false);

    if (_transparentEnabled == true)
        SetTransparentEnabled(true);

    if (_zoom != 100)
        SetZoom(_zoom);

    RefitContent();
    FocusWebView2();

    return S_OK;
}

HRESULT Photino::HandleScriptAddedOnDocumentCreated(HRESULT result, LPCWSTR id)
{
   if (FAILED(result)) return result;

   _scriptId = id ? id : L"";

    return CompleteWebViewInitialization();
}

HRESULT Photino::HandleWebMessageReceived(ICoreWebView2* webview, ICoreWebView2WebMessageReceivedEventArgs* args)
{
    if (!args) return E_POINTER;
    if (!_webMessageReceivedCallback) return S_OK;

    wil::unique_cotaskmem_string message;
    HRESULT hr = args->TryGetWebMessageAsString(&message);
    if (FAILED(hr))
        return hr;

    wil::unique_cotaskmem_string sourceUri;
    hr = args->get_Source(&sourceUri);
    if (FAILED(hr))
        return hr;

    std::string utf8Message = ToUtf8String(message ? PlatformString(message.get()) : PlatformString());
    _webMessageReceivedCallback(utf8Message.c_str());

    return S_OK;
}

HRESULT Photino::HandleWebResourceRequested(ICoreWebView2* webview, ICoreWebView2WebResourceRequestedEventArgs* args)
{
    if (!args) return E_POINTER;

    wil::com_ptr<ICoreWebView2WebResourceRequest> request;
    HRESULT hr = args->get_Request(&request);
    if (FAILED(hr)) return hr;
    if (!request) return E_POINTER;

    wil::unique_cotaskmem_string uri;
    hr = request->get_Uri(&uri);
    if (FAILED(hr)) return hr;
    if (!uri) return E_POINTER;

    PlatformString uriString(uri.get());
    size_t colonPos = uriString.find(L':');

    if (colonPos == PlatformString::npos || colonPos == 0)
        return S_OK;

    PlatformString scheme = uriString.substr(0, colonPos);

    if (!_customSchemeCallback || !IsCustomScheme(scheme))
        return S_OK;

    std::string uriUtf8 = ToUtf8String(uriString);

    int numBytes = 0;
    Utf8String contentType = nullptr;
    void* responseData = _customSchemeCallback(uriUtf8.c_str(), &numBytes, &contentType);

    if (!_webviewEnvironment)
    {
        FreeMemory(responseData);
        FreeString(const_cast<char*>(contentType));
        return E_POINTER;
    }

    HRESULT responseResult = S_OK;
    if (!responseData || numBytes <= 0)
    {
        wil::com_ptr<IStream> emptyStream;
        emptyStream.attach(SHCreateMemStream(nullptr, 0));

        if (!emptyStream)
        {
            FreeMemory(responseData);
            FreeString(const_cast<char*>(contentType));
            return E_OUTOFMEMORY;
        }

        wil::com_ptr<ICoreWebView2WebResourceResponse> response;
        responseResult = _webviewEnvironment->CreateWebResourceResponse(
            emptyStream.get(),
            404,
            L"Not Found",
            nullptr,
            &response);

        if (SUCCEEDED(responseResult))
            responseResult = args->put_Response(response.get());
    }
    else
    {
        PlatformString headers;

        if (contentType && *contentType)
            headers = L"Content-Type: " + ToPlatformString(contentType);

        wil::com_ptr<IStream> dataStream;
        dataStream.attach(SHCreateMemStream(static_cast<const BYTE*>(responseData), static_cast<UINT>(numBytes)));

        if (dataStream)
        {
            wil::com_ptr<ICoreWebView2WebResourceResponse> response;
            responseResult = _webviewEnvironment->CreateWebResourceResponse(
                dataStream.get(),
                200,
                L"OK",
                headers.empty() ? nullptr : headers.c_str(),
                &response);

            if (SUCCEEDED(responseResult))
                responseResult = args->put_Response(response.get());
        }
        else
        {
            responseResult = E_OUTOFMEMORY;
        }
    }

    FreeMemory(responseData);
    FreeString(const_cast<char*>(contentType));

    return responseResult;
}

HRESULT Photino::HandlePermissionRequested(ICoreWebView2* webview, ICoreWebView2PermissionRequestedEventArgs* args)
{
    if (!args) return E_POINTER;

    if (_grantBrowserPermissions)
        return args->put_State(COREWEBVIEW2_PERMISSION_STATE_ALLOW);

    return S_OK;
}

HRESULT Photino::HandleWebViewControllerCreated(HRESULT result, ICoreWebView2Controller* controller)
{
    if (FAILED(result)) return result;
    if (!controller) return E_POINTER;

    HRESULT hr = controller->QueryInterface(&_webviewController);
    if (FAILED(hr)) return hr;

    hr = _webviewController->get_CoreWebView2(&_webviewWindow);
    if (FAILED(hr)) return hr;
    if (!_webviewWindow) return E_POINTER;

    wil::com_ptr<ICoreWebView2Settings> settings;
    hr = _webviewWindow->get_Settings(&settings);
    if (FAILED(hr)) return hr;
    if (!settings) return E_POINTER;

    hr = settings->put_AreHostObjectsAllowed(TRUE);
    if (FAILED(hr)) return hr;
    hr = settings->put_IsScriptEnabled(TRUE);
    if (FAILED(hr)) return hr;
    hr = settings->put_AreDefaultScriptDialogsEnabled(TRUE);
    if (FAILED(hr)) return hr;
    hr = settings->put_IsWebMessageEnabled(TRUE);
    if (FAILED(hr)) return hr;

    EventRegistrationToken webMessageToken;
    hr = _webviewWindow->add_WebMessageReceived(
        Callback<ICoreWebView2WebMessageReceivedEventHandler>(this, &Photino::HandleWebMessageReceived)
        .Get(), &webMessageToken);
    if (FAILED(hr)) return hr;

    hr = _webviewWindow->AddWebResourceRequestedFilter(L"*", COREWEBVIEW2_WEB_RESOURCE_CONTEXT_ALL);
    if (FAILED(hr)) return hr;

    EventRegistrationToken webResourceRequestedToken;
    hr = _webviewWindow->add_WebResourceRequested(
        Callback<ICoreWebView2WebResourceRequestedEventHandler>(this, &Photino::HandleWebResourceRequested)
        .Get(), &webResourceRequestedToken);
    if (FAILED(hr)) return hr;

    EventRegistrationToken permissionRequestedToken;
    hr = _webviewWindow->add_PermissionRequested(
        Callback<ICoreWebView2PermissionRequestedEventHandler>(this, &Photino::HandlePermissionRequested)
        .Get(), &permissionRequestedToken);
    if (FAILED(hr)) return hr;

    hr = _webviewWindow->AddScriptToExecuteOnDocumentCreated(
        L"window.external = { sendMessage: function(message) { window.chrome.webview.postMessage(message); }, receiveMessage: function(callback) { window.chrome.webview.addEventListener('message', function(e) { callback(e.data); }); } };",
        Callback<ICoreWebView2AddScriptToExecuteOnDocumentCreatedCompletedHandler>(this, &Photino::HandleScriptAddedOnDocumentCreated)
            .Get());
    if (FAILED(hr)) return hr;

    return S_OK;
}

HRESULT Photino::HandleWebViewEnvironmentCreated(HRESULT result, ICoreWebView2Environment* environment)
{
    if (FAILED(result)) return result;
    if (!environment) return E_POINTER;

    HRESULT hr = environment->QueryInterface(&_webviewEnvironment);
    if (FAILED(hr)) return hr;

    return environment->CreateCoreWebView2Controller(_hWnd,
        Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(this, &Photino::HandleWebViewControllerCreated)
        .Get());
}

void Photino::AttachWebView()
{
    auto options = Microsoft::WRL::Make<CoreWebView2EnvironmentOptions>();
    if (!options)
    {
        MessageBoxW(_hWnd, L"Failed to allocate WebView2 environment options.", L"Error configuring webview", MB_OK);
        return;
    }

    PlatformString startupString = BuildStartupString();
    if (!startupString.empty())
    {
        HRESULT hr = options->put_AdditionalBrowserArguments(startupString.c_str());
        if (FAILED(hr))
        {
            _com_error err(hr);
            MessageBoxW(_hWnd, err.ErrorMessage(), L"Error configuring webview", MB_OK);
            return;
        }
    }

    PCWSTR runtimePath = g_webview2RuntimePath.empty() ? nullptr : g_webview2RuntimePath.c_str();
    PCWSTR userDataFolder = _temporaryFilesPath.empty() ? nullptr : _temporaryFilesPath.c_str();

    HRESULT envResult = CreateCoreWebView2EnvironmentWithOptions(runtimePath, userDataFolder, options.Get(),
        Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(this, &Photino::HandleWebViewEnvironmentCreated)
        .Get());

    if (FAILED(envResult))
    {
        _com_error err(envResult);
        LPCTSTR errMsg = err.ErrorMessage();
        MessageBoxW(_hWnd, errMsg, L"Error instantiating webview", MB_OK);
    }
}

bool Photino::EnsureWebViewIsInstalled()
{
    LPWSTR versionInfo = nullptr;
    HRESULT hr = GetAvailableCoreWebView2BrowserVersionString(nullptr, &versionInfo);

    if (versionInfo)
        CoTaskMemFree(versionInfo);

    if (FAILED(hr))
        return InstallWebView2();

    return true;
}

bool Photino::InstallWebView2()
{
    const wchar_t* srcUrl = L"https://go.microsoft.com/fwlink/p/?LinkId=2124703";

    wchar_t tempPath[MAX_PATH];
    if (!GetTempPathW(_countof(tempPath), tempPath))
        return false;

    wchar_t destFile[MAX_PATH];
    if (wcscpy_s(destFile, tempPath) != 0)
        return false;

    if (wcscat_s(destFile, L"MicrosoftEdgeWebview2Setup.exe") != 0)
        return false;

    if (URLDownloadToFileW(nullptr, srcUrl, destFile, 0, nullptr) != S_OK)
        return false;

    wchar_t command[MAX_PATH + 3];
    if (swprintf_s(command, L"\"%s\"", destFile) < 0)
    {
        DeleteFileW(destFile);
        return false;
    }

    STARTUPINFOW si{};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi{};

    BOOL success = CreateProcessW(
        nullptr, // No module name (use command line)
        command, // Command line
        nullptr, // Process handle not inheritable
        nullptr, // Thread handle not inheritable
        FALSE,   // Set handle inheritance to FALSE
        0,       // No creation flags
        nullptr, // Use parent's environment block
        nullptr, // Use parent's starting directory
        &si,     // Pointer to STARTUPINFO structure
        &pi);    // Pointer to PROCESS_INFORMATION structure

    if (!success)
    {
        DeleteFileW(destFile);
        return false;
    }

    DWORD waitResult = WaitForSingleObject(pi.hProcess, INFINITE);

    DWORD exitCode = 0;
    BOOL gotExitCode = waitResult == WAIT_OBJECT_0 &&
                       GetExitCodeProcess(pi.hProcess, &exitCode);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    DeleteFileW(destFile);

    return gotExitCode && exitCode == 0;
}

void Photino::RefitContent() const
{
    if (!_webviewController || !_hWnd)
        return;

    RECT bounds{};
    if (!GetClientRect(_hWnd, &bounds))
        return;

    HRESULT hr = _webviewController->put_Bounds(bounds);
    assert(SUCCEEDED(hr));
}

void Photino::FocusWebView2() const
{
    if (_webviewController)
    {
        _webviewController->MoveFocus(COREWEBVIEW2_MOVE_FOCUS_REASON_PROGRAMMATIC);
    }
}

void Photino::NotifyWebView2WindowMove() const
{
    if (_webviewController)
    {
        //MessageBox(nullptr, L"NotifyWebView2WindowMove() was called!", L"", MB_OK);
        _webviewController->NotifyParentWindowPositionChanged();
    }
}

void Photino::ClearBrowserAutoFill() const
{
    if (!_webviewWindow)
        return;

    auto webview15 = _webviewWindow.try_query<ICoreWebView2_15>();
    if (!webview15)
        return;

    wil::com_ptr<ICoreWebView2Profile> profile;
    HRESULT hr = webview15->get_Profile(&profile);
    if (FAILED(hr) || !profile)
        return;

    auto profile2 = profile.try_query<ICoreWebView2Profile2>();
    if (!profile2)
        return;

    COREWEBVIEW2_BROWSING_DATA_KINDS dataKinds =
        COREWEBVIEW2_BROWSING_DATA_KINDS_GENERAL_AUTOFILL |
        COREWEBVIEW2_BROWSING_DATA_KINDS_PASSWORD_AUTOSAVE;

    hr = profile2->ClearBrowsingData(dataKinds,
        Callback<ICoreWebView2ClearBrowsingDataCompletedHandler>(
            [](HRESULT error)-> HRESULT {
                return S_OK;
            }).Get());
    assert(SUCCEEDED(hr));
}

void Photino::SetWebView2RuntimePath(const PlatformString& pathToWebView2)
{
    g_webview2RuntimePath = pathToWebView2;
}

void Photino::Show()
{
    if (!_hWnd)
        std::abort();

    if (!_isAlreadyShown)
    {
        ShowWindow(_hWnd, SW_SHOWDEFAULT);	//causes maximized and minimized to not work
        _isAlreadyShown = true;
    }

    UpdateWindow(_hWnd);

    // Strangely, it only works to create the webview2 *after* the window has been shown,
    // so defer it until here. This unfortunately means you can't call the Navigate methods
    // until the window is shown.
    if (!_webviewController)
    {
        if (!g_webview2RuntimePath.empty() || EnsureWebViewIsInstalled())
            AttachWebView();
        else
            std::abort();
    }
}

bool Photino::RegisterCustomSchemeName(const PlatformString& scheme)
{
    return true;
}
