#include "Photino.h"
#include "Photino.Dialog.h"
#include "Photino.Strings.h"
#include "Photino.Windows.DarkMode.h"
#include "Photino.Windows.State.h"
#include "Photino.Windows.ToastHandler.h"

#include <algorithm>
#include <atomic>
#include <cassert>
#include <comdef.h>
#include <map>
#include <memory>
#include <mutex>

#include <Windows.h>

#define WM_USER_INVOKE (WM_USER + 0x0002)

using namespace WinToastLib;
using namespace PhotinoX::Native;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

namespace
{
    constexpr LPCWSTR CLASS_NAME = L"PhotinoX";

    HINSTANCE g_hInstance = nullptr;
    std::atomic g_messageLoopRunning{ false };
    HWND g_uiThreadWindowHandle = nullptr;
    std::atomic g_isShuttingDown{ false };
    std::map<HWND, Photino*> g_hwndToPhotino;
}

extern PlatformString g_webview2RuntimePath;

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

Photino::Photino(PhotinoInitParams* initParams) : platform_(std::make_unique<WindowsState>())
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

    platform_->sizeLimits.minWidth = (std::max)(0, initParams->MinWidth);
    platform_->sizeLimits.minHeight = (std::max)(0, initParams->MinHeight);
    platform_->sizeLimits.maxWidth = (std::max)(0, initParams->MaxWidth);
    platform_->sizeLimits.maxHeight = (std::max)(0, initParams->MaxHeight);

    if (platform_->sizeLimits.maxWidth > 0 && platform_->sizeLimits.minWidth > platform_->sizeLimits.maxWidth)    platform_->sizeLimits.maxWidth = platform_->sizeLimits.minWidth;
    if (platform_->sizeLimits.maxHeight > 0 && platform_->sizeLimits.minHeight > platform_->sizeLimits.maxHeight) platform_->sizeLimits.maxHeight = platform_->sizeLimits.minHeight;

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
    platform_->hWnd = CreateWindowExW(
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

    if (!platform_->hWnd)
        std::abort();

    g_hwndToPhotino[platform_->hWnd] = this;

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

        platform_->toastHandler = new WinToastHandler(this);
        WinToast::instance()->initialize();
    }

    _dialog = new PhotinoDialog(this);

    platform_->isAlreadyShown = initParams->Minimized || initParams->Maximized;
    Show();
}

Photino::~Photino()
{
    delete _dialog;
    _dialog = nullptr;

    delete platform_->toastHandler;
    platform_->toastHandler = nullptr;
}

void Photino::ApplySizeLimits(MINMAXINFO& info) const noexcept
{
    if (platform_->sizeLimits.minWidth > 0)   info.ptMinTrackSize.x = platform_->sizeLimits.minWidth;
    if (platform_->sizeLimits.minHeight > 0)  info.ptMinTrackSize.y = platform_->sizeLimits.minHeight;
    if (platform_->sizeLimits.maxWidth > 0)   info.ptMaxTrackSize.x = platform_->sizeLimits.maxWidth;
    if (platform_->sizeLimits.maxHeight > 0)  info.ptMaxTrackSize.y = platform_->sizeLimits.maxHeight;
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
            if (photino)
            {
                photino->CloseWebView();
                photino->InvokeClose();
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

    case WM_NCDESTROY:
    {
        if (const auto it = g_hwndToPhotino.find(hwnd); it != g_hwndToPhotino.end())
        {
            auto photino = it->second;
            g_hwndToPhotino.erase(it);

            delete photino;
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

void Photino::GetNotificationsEnabled(bool* enabled) const
{
    assert(enabled);
    if (!enabled) return;

    *enabled = _notificationsEnabled;
}


void Photino::ShowNotification(const PlatformString& title, const PlatformString& message) const
{
    if (!_notificationsEnabled || !platform_->toastHandler || !WinToast::isCompatible())
        return;

    WinToastTemplate toast = WinToastTemplate(WinToastTemplate::ImageAndText02);
    toast.setTextField(title, WinToastTemplate::FirstLine);
    toast.setTextField(message, WinToastTemplate::SecondLine);

    if (!_iconFileName.empty())
        toast.setImagePath(_iconFileName);

    INT64 result = WinToast::instance()->showToast(toast, platform_->toastHandler);
    assert(result >= 0);
}

void Photino::WaitForExit() const
{
    assert(platform_->hWnd);
    if (!platform_->hWnd) return;

    bool expected = false;
    if (!g_messageLoopRunning.compare_exchange_strong(expected, true, std::memory_order_acq_rel)) return;

    g_isShuttingDown.store(false, std::memory_order_release);
    g_uiThreadWindowHandle = platform_->hWnd;

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

void Photino::Invoke(InvokeCallback callback) const
{
    assert(platform_->hWnd && callback);
    if (!platform_->hWnd || !callback) return;

    if (g_isShuttingDown.load(std::memory_order_acquire)) return;

    DWORD windowThreadId = GetWindowThreadProcessId(platform_->hWnd, nullptr);
    DWORD currentThreadId = GetCurrentThreadId();
    if (windowThreadId == currentThreadId)
    {
        callback();
        return;
    }

    if (!IsWindow(platform_->hWnd))
        return;

    // Block until the callback is actually executed and completed
    LRESULT result = SendMessageW(platform_->hWnd, WM_USER_INVOKE, reinterpret_cast<WPARAM>(callback), 0);
}

void Photino::Show()
{
    if (!platform_->hWnd)
        std::abort();

    if (!platform_->isAlreadyShown)
    {
        ShowWindow(platform_->hWnd, SW_SHOWDEFAULT); // causes maximized and minimized to not work
        platform_->isAlreadyShown = true;
    }

    UpdateWindow(platform_->hWnd);

    // Strangely, it only works to create the webview2 *after* the window has been shown,
    // so defer it until here. This unfortunately means you can't call the Navigate methods
    // until the window is shown.
    if (!platform_->webViewController)
    {
        if (!g_webview2RuntimePath.empty() || EnsureWebViewIsInstalled())
            AttachWebView();
        else
            std::abort();
    }
}
