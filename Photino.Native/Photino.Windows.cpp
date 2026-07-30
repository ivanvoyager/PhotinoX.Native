#include "Photino.h"
#include "Photino.Dialog.h"
#include "Photino.Enums.h"
#include "Photino.Strings.h"
#include "Photino.Windows.DarkMode.h"
#include "Photino.Windows.State.h"
#include "Photino.Windows.ToastHandler.h"
#include "Photino.Application.h"

#include <algorithm>
#include <cassert>
#include <memory>

#include <Windows.h>

using namespace WinToastLib;
using namespace PhotinoX::Native;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

namespace
{
    constexpr LPCWSTR CLASS_NAME = L"PhotinoX";

    HINSTANCE g_hInstance = nullptr;

    Photino* GetPhotino(const HWND hwnd) noexcept
    {
        return reinterpret_cast<Photino*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    }
}

extern PlatformString g_webview2RuntimePath;

const HBRUSH darkBrush = CreateSolidBrush(RGB(0, 0, 0));
const HBRUSH lightBrush = CreateSolidBrush(RGB(255, 255, 255));

void Photino::Register(const HINSTANCE hInstance)
{
    InitDarkModeSupport();

    //g_hInstance = GetModuleHandleW(nullptr);
    g_hInstance = hInstance;

    assert(g_hInstance == GetModuleHandleW(nullptr));

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

    InitializeFromInitParams(initParams);

    const auto startupWindowState = options_.windowState;
    options_.windowState = PhotinoWindowState::Normal;
    const bool startFullScreen = startupWindowState == PhotinoWindowState::FullScreen;

    platform_->sizeLimits.minWidth = (std::max)(0, initParams->MinWidth);
    platform_->sizeLimits.minHeight = (std::max)(0, initParams->MinHeight);
    platform_->sizeLimits.maxWidth = (std::max)(0, initParams->MaxWidth);
    platform_->sizeLimits.maxHeight = (std::max)(0, initParams->MaxHeight);

    if (platform_->sizeLimits.maxWidth > 0 && platform_->sizeLimits.minWidth > platform_->sizeLimits.maxWidth)    platform_->sizeLimits.maxWidth = platform_->sizeLimits.minWidth;
    if (platform_->sizeLimits.maxHeight > 0 && platform_->sizeLimits.minHeight > platform_->sizeLimits.maxHeight) platform_->sizeLimits.maxHeight = platform_->sizeLimits.minHeight;

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

    if (options_.chromeless)
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

    HWND ownerHwnd = options_.useNativeWindowOwner && parent_ ? parent_->GetHwnd() : nullptr;

    //Create the window
    HWND hWnd = CreateWindowExW(
        options_.transparentEnabled ? WS_EX_LAYERED : 0,        // WS_EX_OVERLAPPEDWINDOW, //An optional extended window style.
        CLASS_NAME,                                             // Window class
        options_.windowTitle.c_str(),                           // Window text
        options_.chromeless ? WS_POPUP : WS_OVERLAPPEDWINDOW,   // Window style

        // Size and position
        initParams->Left, initParams->Top, initParams->Width, initParams->Height,

        ownerHwnd,  // Parent window handle
        nullptr,    // Menu
        g_hInstance,// Instance handle
        this        // Additional application data
    );

    assert(platform_->hWnd == hWnd);
    platform_->hWnd = hWnd;

    if (!platform_->hWnd)
        std::abort();

    RefreshWindowIconsForDpi(0);

    if (initParams->CenterOnInitialize)
        Center();

    switch (startupWindowState)
    {
    case PhotinoWindowState::Maximized:
        platform_->initialShowCommand = SW_SHOWMAXIMIZED;
        break;
    case PhotinoWindowState::Minimized:
        platform_->initialShowCommand = SW_SHOWMINIMIZED;
        break;
    default:
        platform_->initialShowCommand = SW_SHOWDEFAULT;
        break;
    }

    SetResizable(initParams->Resizable);

    if (initParams->Topmost)
        SetTopmost(true);

    if (options_.notificationsEnabled)
    {
        WinToast::instance()->setAppName(options_.windowTitle);
        if (!options_.notificationRegistrationId.empty())
            WinToast::instance()->setAppUserModelId(options_.notificationRegistrationId);
        else
            WinToast::instance()->setAppUserModelId(options_.windowTitle);

        platform_->toastHandler = new WinToastHandler(this);
        WinToast::instance()->initialize();
    }

    dialog_ = new PhotinoDialog(this);

    suppressWindowStateCallbacks_ = true;

    Show();
    UpdateWindowState();

    if (startFullScreen)
        SetFullScreen(true);

    suppressWindowStateCallbacks_ = false;

    // Photino creates WebView2 after the native window is shown because creating it
    // earlier has historically caused initialization/display issues.
    if (!EnsureWebViewAttached())
        std::abort();

    platform_->suppressWindowCallbacks = false;
}

Photino::~Photino()
{
    delete dialog_;
    dialog_ = nullptr;

    delete platform_->toastHandler;
    platform_->toastHandler = nullptr;

    if (platform_->ownedSmallIcon)
    {
        DestroyIcon(platform_->ownedSmallIcon);
        platform_->ownedSmallIcon = nullptr;
    }

    if (platform_->ownedBigIcon)
    {
        DestroyIcon(platform_->ownedBigIcon);
        platform_->ownedBigIcon = nullptr;
    }
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

    case WM_NCCREATE:
    {
        const auto createStruct = reinterpret_cast<CREATESTRUCTW*>(lParam);
        if (!createStruct)
            return FALSE;

        auto photino = static_cast<Photino*>(createStruct->lpCreateParams);
        if (!photino)
            return FALSE;

        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(photino));

        photino->Platform().hWnd = hwnd;

        break;
    }

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

        if (const auto photino = GetPhotino(hwnd))
            photino->RefreshWindowIconsForDpi(LOWORD(wParam));

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
        auto photino = GetPhotino(hwnd);
        if (!photino)
            return 0;

        if (LOWORD(wParam) == WA_INACTIVE)
        {
            photino->InvokeFocusOut();
        }
        else
        {
            photino->FocusWebView2();
            photino->InvokeFocusIn();
        }

        return 0;
    }

    case WM_CLOSE:
    {
        auto photino = GetPhotino(hwnd);
        if (!photino)
            break;

        if (!PhotinoApplication::Instance().IsShuttingDown())
        {
            if (photino && photino->InvokeClosing())
                return 0;
        }

        DestroyWindow(hwnd);
        return 0;
    }

    case WM_DESTROY:
    {
        auto photino = GetPhotino(hwnd);
        if (photino)
        {
            photino->CloseWebView();
            photino->InvokeClose();
        }

        return 0;
    }

    case WM_NCDESTROY:
    {
        auto photino = GetPhotino(hwnd);

        SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);

        delete photino;
        return 0;
    }

    case WM_GETMINMAXINFO:
    {
        auto photino = GetPhotino(hwnd);
        if (photino)
            photino->ApplySizeLimits(*reinterpret_cast<MINMAXINFO*>(lParam));

        return 0;
    }

    case WM_WINDOWPOSCHANGED:
    {
        auto photino = GetPhotino(hwnd);
        if (photino)
            photino->UpdateWindowState();

        break;
    }

    case WM_SIZE:
    {
        auto photino = GetPhotino(hwnd);
        if (!photino) return 0;

        photino->UpdateWindowState();

        if (photino->Platform().suppressWindowCallbacks)
            return 0;

        photino->RefitContent();

        int width = 0, height = 0;
        photino->GetSize(&width, &height);
        photino->InvokeResize(width, height);

        return 0;
    }

    case WM_MOVE:
    {
        auto photino = GetPhotino(hwnd);
        if (!photino) return 0;

        if (photino->Platform().suppressWindowCallbacks)
            return 0;

        // photino->NotifyWebView2WindowMove();
        // photino->RefitContent();

        int x = 0, y = 0;
        photino->GetPosition(&x, &y);
        photino->InvokeMove(x, y);

        return 0;
    }

    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void Photino::GetNotificationsEnabled(bool* enabled) const
{
    assert(enabled);
    if (!enabled) return;

    *enabled = options_.notificationsEnabled;
}


void Photino::ShowNotification(const PlatformString& title, const PlatformString& message) const
{
    if (!options_.notificationsEnabled || !platform_->toastHandler || !WinToast::isCompatible())
        return;

    WinToastTemplate toast = WinToastTemplate(WinToastTemplate::ImageAndText02);
    toast.setTextField(title, WinToastTemplate::FirstLine);
    toast.setTextField(message, WinToastTemplate::SecondLine);

    if (!options_.iconFileName.empty())
        toast.setImagePath(options_.iconFileName);

    INT64 result = WinToast::instance()->showToast(toast, platform_->toastHandler);
    assert(result >= 0);
}