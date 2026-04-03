#include "Photino.h"
#include "Photino.Dialog.h"
#include "Photino.Windows.DarkMode.h"
#include "Photino.Windows.ToastHandler.h"

#include <mutex>
#include <condition_variable>
#include <comdef.h>
#include <Shlwapi.h>
#include <wrl.h>
#include <windows.h>
#include <algorithm>
#include <cassert>
#include <WebView2EnvironmentOptions.h>
#include <Shellscalingapi.h>

#pragma comment(lib, "Shcore.lib")
#pragma comment(lib, "Urlmon.lib")
#pragma warning(disable: 4996)		//disable warning about wcscpy vs. wcscpy_s

#define WM_USER_INVOKE (WM_USER + 0x0002)

using namespace WinToastLib;
using namespace Microsoft::WRL;
using namespace PhotinoX::Native;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LPCWSTR CLASS_NAME = L"PhotinoX";
std::mutex invokeLockMutex;
HINSTANCE Photino::_hInstance;
HWND messageLoopRootWindowHandle = nullptr;
std::map<HWND, Photino*> hwndToPhotino;
wchar_t _webview2RuntimePath[MAX_PATH];


struct InvokeWaitInfo
{
    std::condition_variable completionNotifier;
    bool isCompleted;
};

struct ShowMessageParams
{
    std::wstring title;
    std::wstring body;
    UINT type = 0;
};


const HBRUSH darkBrush = CreateSolidBrush(RGB(0, 0, 0));
const HBRUSH lightBrush = CreateSolidBrush(RGB(255, 255, 255));

void Photino::Register(const HINSTANCE hInstance)
{
    InitDarkModeSupport();

    _hInstance = hInstance;

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

    RegisterClassEx(&wcx);

    SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
}

Photino::Photino(PhotinoInitParams* initParams)
{
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
        MessageBox(nullptr, msg, L"Native Initialization Failed", MB_OK);
        exit(0);
    }

    _windowTitle = new wchar_t[256];
    if (initParams->Title != nullptr)
    {
        AutoString wTitle = ToUTF16String(initParams->Title);
        if (initParams->NotificationsEnabled)
        {
            WinToast::instance()->setAppName(wTitle);
            if (_notificationRegistrationId == nullptr)
                WinToast::instance()->setAppUserModelId(wTitle);
        }
        wcscpy(_windowTitle, wTitle);
    }
    else
        _windowTitle[0] = 0;

    _startUrl = nullptr;
    if (initParams->StartUrl != nullptr)
    {
        _startUrl = new wchar_t[2048];
        if (_startUrl == nullptr) exit(0);
        //AutoString wStartUrl = ToUTF16String(initParams->StartUrl);	//Conversion is done in Navigate method. Don't do it twice
        //wcscpy(_startUrl, wStartUrl);
        wcscpy(_startUrl, initParams->StartUrl);
    }

    _startString = nullptr;
    if (initParams->StartString != nullptr)
    {
        //AutoString wStartString = ToUTF16String(initParams->StartString);	//Conversion is done in Navigate method. Don't do it twice
        //_startString = new wchar_t[wcslen(wStartString) + 1];
        _startString = new wchar_t[wcslen(initParams->StartString) + 1];
        if (_startString == nullptr) exit(0);
        //wcscpy(_startString, wStartString);
        wcscpy(_startString, initParams->StartString);
    }

    _temporaryFilesPath = nullptr;
    if (initParams->TemporaryFilesPath != nullptr)
    {
        _temporaryFilesPath = new wchar_t[256];
        if (_temporaryFilesPath == nullptr) exit(0);
        AutoString wTemporaryFilesPath = ToUTF16String(initParams->TemporaryFilesPath);
        wcscpy(_temporaryFilesPath, wTemporaryFilesPath);
    }

    _userAgent = nullptr;
    if (initParams->UserAgent != nullptr)
    {
        AutoString wUserAgent = ToUTF16String(initParams->UserAgent);
        _userAgent = new wchar_t[wcslen(wUserAgent) + 1];
        if (_userAgent == nullptr) exit(0);
        wcscpy(_userAgent, wUserAgent);
    }

    _browserControlInitParameters = nullptr;
    if (initParams->BrowserControlInitParameters != nullptr)
    {
        AutoString wBrowserControlInitParameters = ToUTF16String(initParams->BrowserControlInitParameters);
        _browserControlInitParameters = new wchar_t[wcslen(wBrowserControlInitParameters) + 1];
        if (_browserControlInitParameters == nullptr) exit(0);
        wcscpy(_browserControlInitParameters, wBrowserControlInitParameters);
    }

    _notificationRegistrationId = nullptr;
    if (initParams->NotificationRegistrationId != nullptr)
    {
        AutoString wNotificationRegistrationId = ToUTF16String(initParams->NotificationRegistrationId);
        _notificationRegistrationId = new wchar_t[wcslen(wNotificationRegistrationId) + 1];
        if (_notificationRegistrationId == nullptr) exit(0);
        wcscpy(_notificationRegistrationId, wNotificationRegistrationId);
    }


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

    _zoom = initParams->Zoom;
    _minWidth = initParams->MinWidth;
    _minHeight = initParams->MinHeight;
    _maxWidth = initParams->MaxWidth;
    _maxHeight = initParams->MaxHeight;

    //these handlers are ALWAYS hooked up
    _webMessageReceivedCallback = reinterpret_cast<WebMessageReceivedCallback>(initParams->WebMessageReceivedHandler);
    _resizedCallback = reinterpret_cast<ResizedCallback>(initParams->ResizedHandler);
    _maximizedCallback = reinterpret_cast<MaximizedCallback>(initParams->MaximizedHandler);
    _restoredCallback = reinterpret_cast<RestoredCallback>(initParams->RestoredHandler);
    _minimizedCallback = reinterpret_cast<MinimizedCallback>(initParams->MinimizedHandler);
    _movedCallback = reinterpret_cast<MovedCallback>(initParams->MovedHandler);
    _closingCallback = reinterpret_cast<ClosingCallback>(initParams->ClosingHandler);
    _closedCallback = reinterpret_cast<ClosedCallback>(initParams->ClosedHandler);
    _focusInCallback = reinterpret_cast<FocusInCallback>(initParams->FocusInHandler);
    _focusOutCallback = reinterpret_cast<FocusOutCallback>(initParams->FocusOutHandler);
    _customSchemeCallback = reinterpret_cast<WebResourceRequestedCallback>(initParams->CustomSchemeHandler);

    //copy strings from the fixed size array passed, but only if they have a value.
    for (int i = 0; i < 16; ++i)
    {
        if (initParams->CustomSchemeNames[i] != nullptr)
        {
            wchar_t* name = new wchar_t[50];
            AutoString wCustomSchemeNames = ToUTF16String(initParams->CustomSchemeNames[i]);
            wcscpy(name, wCustomSchemeNames);
            _customSchemeNames.push_back(name);
        }
    }

    _parent = initParams->ParentInstance;

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

    if (initParams->FullScreen == true)
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

    if (initParams->Height > initParams->MaxHeight) initParams->Height = initParams->MaxHeight;
    if (initParams->Height < initParams->MinHeight && initParams->MinHeight > 0) initParams->Height = initParams->MinHeight;
    if (initParams->Width > initParams->MaxWidth) initParams->Width = initParams->MaxWidth;
    if (initParams->Width < initParams->MinWidth && initParams->MinWidth > 0) initParams->Width = initParams->MinWidth;

    //Create the window
    _hWnd = CreateWindowEx(
        initParams->Transparent ? WS_EX_LAYERED : 0, //WS_EX_OVERLAPPEDWINDOW, //An optional extended window style.
        CLASS_NAME,					//Window class
        _windowTitle,		//Window text
        initParams->Chromeless || initParams->FullScreen ? WS_POPUP : WS_OVERLAPPEDWINDOW,	//Window style

        // Size and position
        initParams->Left, initParams->Top, initParams->Width, initParams->Height,

        nullptr,    //Parent window handle
        nullptr,    //Menu
        _hInstance, //Instance handle
        this        //Additional application data
    );
    hwndToPhotino[_hWnd] = this;

    if (initParams->WindowIconFile != nullptr)
    {
        AutoString wWindowIconFile = ToUTF16String(initParams->WindowIconFile);
        SetIconFile(wWindowIconFile);
    }

    if (initParams->CenterOnInitialize)
        Center();

    if (initParams->Minimized)
        SetMinimized(true);

    if (initParams->Maximized)
        SetMaximized(true);

    //if (initParams->Resizable == false)
    SetResizable(initParams->Resizable);

    if (initParams->Topmost)
        SetTopmost(true);

    if (initParams->NotificationsEnabled)
    {
        if (_notificationRegistrationId != nullptr)
            WinToast::instance()->setAppUserModelId(_notificationRegistrationId);

        this->_toastHandler = new WinToastHandler(this);
        WinToast::instance()->initialize();
    }

    _dialog = new PhotinoDialog(this);

    bool isAlreadyShown = initParams->Minimized || initParams->Maximized;
    Show(isAlreadyShown);
}

Photino::~Photino()
{
    if (_startUrl != nullptr) delete[]_startUrl;
    if (_startString != nullptr) delete[]_startString;
    if (_temporaryFilesPath != nullptr) delete[]_temporaryFilesPath;
    if (_windowTitle != nullptr) delete[]_windowTitle;
    if (_notificationsEnabled && _toastHandler != nullptr) delete _toastHandler;
}

HWND Photino::getHwnd() const
{
    return _hWnd;
}

LRESULT CALLBACK WindowProc(const HWND hwnd, const UINT uMsg, const WPARAM wParam, const LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_CREATE:
    {
        EnableDarkMode(hwnd, true);
        if (IsDarkModeEnabled())
            RefreshNonClientArea(hwnd);
        break;
    }
    case WM_DPICHANGED:
    {
        UINT dpiX = HIWORD(wParam);
        UINT dpiY = LOWORD(wParam);

        RECT* newWindowRect = (RECT*)lParam;

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

        break;
    }
    case WM_THEMECHANGED:
    {
        EnableDarkMode(hwnd, IsDarkModeEnabled());
        RefreshNonClientArea(hwnd);
        InvalidateRect(hwnd, nullptr, TRUE);
        break;
    }
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        // Fill the background with the current theme color
        if (IsDarkModeEnabled())
        {
            FillRect(hdc, &ps.rcPaint, darkBrush);
            //SetTextColor(hdc, RGB(255,255,255));
        }
        else
        {
            FillRect(hdc, &ps.rcPaint, lightBrush);
            //SetTextColor(hdc, RGB(0, 0, 0));
        }

        // Draw some text
        //SetBkMode(hdc, TRANSPARENT);
        //TextOut(hdc, 10, 10, L"Hello, World! (Dynamic Theme)", 31);

        EndPaint(hwnd, &ps);
        break;
    }
    case WM_ACTIVATE:
    {
        Photino* Photino = hwndToPhotino[hwnd];
        if (LOWORD(wParam) == WA_INACTIVE)
        {
            Photino->InvokeFocusOut();
        }
        else
        {
            Photino->FocusWebView2();
            Photino->InvokeFocusIn();

            return 0;
        }
        break;
    }
    case WM_CLOSE:
    {
        Photino* Photino = hwndToPhotino[hwnd];
        if (Photino)
        {
            bool doNotClose = Photino->InvokeClosing();
            if (doNotClose)
                return 0;
            DestroyWindow(hwnd);
        }

        return 0;
    }
    case WM_DESTROY:
    {
        Photino* Photino = hwndToPhotino[hwnd];
        if (Photino)
        {
            Photino->CloseWebView();
            Photino->InvokeClose();
        }
        // Only terminate the message loop if the window being closed is the one that
        // started the message loop
        hwndToPhotino.erase(hwnd);
        if (hwnd == messageLoopRootWindowHandle)
            PostQuitMessage(0);

        return 0;
    }
    case WM_USER_INVOKE:
    {
        auto callback = reinterpret_cast<InvokeCallback>(wParam);
        callback();
        auto waitInfo = reinterpret_cast<InvokeWaitInfo*>(lParam);
        {
            std::lock_guard<std::mutex> guard(invokeLockMutex);
            waitInfo->isCompleted = true;
        }
        waitInfo->completionNotifier.notify_one();
        //delete waitInfo; ?
        return 0;
    }
    case WM_GETMINMAXINFO:
    {
        Photino* Photino = hwndToPhotino[hwnd];
        if (Photino == nullptr)
            return 0;

        MINMAXINFO* mmi = (MINMAXINFO*)lParam;
        if (Photino->_minWidth > 0)
            mmi->ptMinTrackSize.x = Photino->_minWidth;
        if (Photino->_minHeight > 0)
            mmi->ptMinTrackSize.y = Photino->_minHeight;
        if (Photino->_maxWidth < INT_MAX)
            mmi->ptMaxTrackSize.x = Photino->_maxWidth;
        if (Photino->_maxHeight < INT_MAX)
            mmi->ptMaxTrackSize.y = Photino->_maxHeight;
        return 0;
    }
    case WM_SIZE:
    {
        Photino* Photino = hwndToPhotino[hwnd];
        if (Photino)
        {
            Photino->RefitContent();
            int width, height;
            Photino->GetSize(&width, &height);
            Photino->InvokeResize(width, height);

            if (LOWORD(wParam) == SIZE_MAXIMIZED) {
                Photino->InvokeMaximized();
            }
            else if (LOWORD(wParam) == SIZE_RESTORED) {
                Photino->InvokeRestored();
            }
            else if (LOWORD(wParam) == SIZE_MINIMIZED) {
                Photino->InvokeMinimized();
            }
        }
        return 0;
    }
    case WM_MOVE:
    {
        Photino* Photino = hwndToPhotino[hwnd];
        if (Photino)
        {
            //Photino->NotifyWebView2WindowMove();
            //Photino->RefitContent();

            int x, y;
            Photino->GetPosition(&x, &y);
            Photino->InvokeMove(x, y);
        }
        return 0;
    }
    case WM_MOVING:
    {
        Photino* Photino = hwndToPhotino[hwnd];
        if (Photino)
        {
            //Photino->NotifyWebView2WindowMove();
            //Photino->RefitContent();
        }
    }
    break;
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void Photino::CloseWebView()
{
    if (_webviewController != nullptr)
    {
        _webviewController->Close();
        _webviewController = nullptr;
    }

    if (_webviewWindow != nullptr)
    {
        _webviewWindow->Stop();
        _webviewWindow = nullptr;
    }

    if (_webviewEnvironment != nullptr)
    {
        _webviewEnvironment = nullptr;
    }
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

    SetWindowPos(_hWnd, nullptr, left, top, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
}

void Photino::Close() const
{
    assert(_hWnd);
    if (!_hWnd) return;

    PostMessage(_hWnd, WM_CLOSE, 0, 0);
}

void Photino::GetTransparentEnabled(bool* enabled) const
{
    assert(enabled && _webviewController);
    if (!enabled) return;

    *enabled = _transparentEnabled;

    if (!_webviewController) return;

    ICoreWebView2Controller2* controller2 = nullptr;
    if (FAILED(_webviewController->QueryInterface(&controller2)) || !controller2) return;

    COREWEBVIEW2_COLOR backgroundColor{};
    if (SUCCEEDED(controller2->get_DefaultBackgroundColor(&backgroundColor)))
        *enabled = (backgroundColor.A == 0);

    controller2->Release();
}

void Photino::GetContextMenuEnabled(bool* enabled) const
{
    assert(enabled && _webviewWindow);
    if (!enabled) return;

    *enabled = _contextMenuEnabled;

    if (!_webviewWindow) return;

    ICoreWebView2Settings* settings = nullptr;
    if (FAILED(_webviewWindow->get_Settings(&settings)) || !settings) return;

    BOOL value = FALSE;
    if (SUCCEEDED(settings->get_AreDefaultContextMenusEnabled(&value)))
        *enabled = (value == TRUE);

    settings->Release();
}

void Photino::GetZoomEnabled(bool* enabled) const
{
    assert(enabled && _webviewWindow);
    if (!enabled) return;

    *enabled = _zoomEnabled;

    if (!_webviewWindow) return;

    ICoreWebView2Settings* settings = nullptr;
    if (FAILED(_webviewWindow->get_Settings(&settings)) || !settings) return;

    BOOL value = FALSE;
    if (SUCCEEDED(settings->get_IsZoomControlEnabled(&value)))
        *enabled = (value == TRUE);

    settings->Release();
}

void Photino::GetDevToolsEnabled(bool* enabled) const
{
    assert(enabled && _webviewWindow);
    if (!enabled) return;

    *enabled = _devToolsEnabled;

    if (!_webviewWindow) return;

    ICoreWebView2Settings* settings = nullptr;
    if (FAILED(_webviewWindow->get_Settings(&settings)) || !settings) return;

    BOOL value = FALSE;
    if (SUCCEEDED(settings->get_AreDevToolsEnabled(&value)))
        *enabled = (value == TRUE);

    settings->Release();
}

void Photino::GetFullScreen(bool* fullScreen) const
{
    assert(fullScreen && _hWnd);
    if (!fullScreen || !_hWnd) return;

    *fullScreen = false;
    LONG lStyles = GetWindowLong(_hWnd, GWL_STYLE);
    if (lStyles & WS_POPUP) *fullScreen = true;
}

void Photino::GetGrantBrowserPermissions(bool* grant) const
{
    assert(grant);
    if (!grant) return;

    *grant = _grantBrowserPermissions;
}

AutoString Photino::GetUserAgent() const
{
    return this->_userAgent;
}

void Photino::GetMediaAutoplayEnabled(bool* enabled) const
{
    assert(enabled);
    if (!enabled) return;

    *enabled = this->_mediaAutoplayEnabled;
}

void Photino::GetFileSystemAccessEnabled(bool* enabled) const
{
    assert(enabled);
    if (!enabled) return;

    *enabled = this->_fileSystemAccessEnabled;
}

void Photino::GetWebSecurityEnabled(bool* enabled) const
{
    assert(enabled);
    if (!enabled) return;

    *enabled = this->_webSecurityEnabled;
}

void Photino::GetJavascriptClipboardAccessEnabled(bool* enabled) const
{
    assert(enabled);
    if (!enabled) return;

    *enabled = this->_javascriptClipboardAccessEnabled;
}

void Photino::GetMediaStreamEnabled(bool* enabled) const
{
    assert(enabled);
    if (!enabled) return;

    *enabled = this->_mediaStreamEnabled;
}

void Photino::GetSmoothScrollingEnabled(bool* enabled) const
{
    assert(enabled);
    if (!enabled) return;

    *enabled = this->_smoothScrollingEnabled;
}

void Photino::GetIgnoreCertificateErrorsEnabled(bool* enabled) const
{
    assert(enabled);
    if (!enabled) return;

    *enabled = this->_ignoreCertificateErrorsEnabled;
}

void Photino::GetNotificationsEnabled(bool* enabled) const
{
    assert(enabled);
    if (!enabled) return;

    *enabled = this->_notificationsEnabled;
}

AutoString Photino::GetIconFileName() const
{
    return this->_iconFileName;
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

AutoString Photino::GetTitle() const
{
    //int titleLength = GetWindowTextLength(_hWnd) + 1;
    //wchar_t* title = new wchar_t[titleLength];
    //GetWindowText(_hWnd, title, titleLength);
    //MessageBox(nullptr, title, L"", MB_OK);
    return _windowTitle;
}

void Photino::GetTopmost(bool* topmost) const
{
    assert(topmost && _hWnd);
    if (!topmost) return;

    *topmost = false;

    if (!_hWnd) return;

    LONG lStyles = GetWindowLong(_hWnd, GWL_EXSTYLE);
    if (lStyles & WS_EX_TOPMOST) *topmost = true;
}

void Photino::GetZoom(int* zoom) const
{
    assert(zoom && _webviewController);
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
void Photino::NavigateToString(AutoString content)
{
    assert(_webviewWindow);
    if (!_webviewWindow) return;

    content = ToUTF16String(content);
    _webviewWindow->NavigateToString(content);
}

void Photino::NavigateToUrl(AutoString url)
{
    assert(_webviewWindow);
    if (!_webviewWindow) return;

    url = ToUTF16String(url);
    _webviewWindow->Navigate(url);
}

void Photino::Restore()
{
    assert(_hWnd);
    if (!_hWnd)  return;

    ShowWindow(_hWnd, SW_RESTORE);
}

void Photino::SendWebMessage(AutoString message)
{
    assert(_webviewWindow);
    if (!_webviewWindow) return;

    message = ToUTF16String(message);
    _webviewWindow->PostWebMessageAsString(message);
}

void Photino::SetTransparentEnabled(const bool enabled)
{
    _transparentEnabled = enabled;
    assert(_webviewController && _webviewWindow);
    if (!_webviewController || !_webviewWindow) return;

    ICoreWebView2Controller2* controller2 = nullptr;
    if (FAILED(_webviewController->QueryInterface(&controller2)) || !controller2) return;

    COREWEBVIEW2_COLOR backgroundColor{};
    if (SUCCEEDED(controller2->get_DefaultBackgroundColor(&backgroundColor)))
    {
        backgroundColor.A = enabled ? 0 : 255;
        controller2->put_DefaultBackgroundColor(backgroundColor);
    }

    controller2->Release();
    _webviewWindow->Reload();
}

void Photino::SetContextMenuEnabled(const bool enabled)
{
    _contextMenuEnabled = enabled;
    assert(_webviewWindow);
    if (!_webviewWindow) return;

    ICoreWebView2Settings* settings = nullptr;
    if (FAILED(_webviewWindow->get_Settings(&settings)) || !settings) return;

    settings->put_AreDefaultContextMenusEnabled(enabled ? TRUE : FALSE);
    settings->Release();

    _webviewWindow->Reload();
}

void Photino::SetZoomEnabled(const bool enabled)
{
    _zoomEnabled = enabled;
    assert(_webviewWindow);
    if (!_webviewWindow) return;

    ICoreWebView2Settings* settings = nullptr;
    if (FAILED(_webviewWindow->get_Settings(&settings)) || !settings) return;

    settings->put_IsZoomControlEnabled(enabled ? TRUE : FALSE);
    settings->Release();

    _webviewWindow->Reload();
}

void Photino::SetDevToolsEnabled(const bool enabled)
{
    _devToolsEnabled = enabled;
    assert(_webviewWindow);
    if (!_webviewWindow) return;

    ICoreWebView2Settings* settings = nullptr;
    if (FAILED(_webviewWindow->get_Settings(&settings)) || !settings) return;

    settings->put_AreDevToolsEnabled(enabled ? TRUE : FALSE);
    settings->Release();

    _webviewWindow->Reload();
}

void Photino::SetFullScreen(const bool fullScreen)
{
    assert(_hWnd);
    if (!_hWnd) return;

    LONG_PTR style = GetWindowLongPtr(_hWnd, GWL_STYLE);
    if (fullScreen)
    {
        style |= WS_POPUP;
        style &= ~WS_OVERLAPPEDWINDOW;
    }
    else
    {
        style |= WS_OVERLAPPEDWINDOW;
        style &= ~WS_POPUP;
    }

    SetWindowLongPtr(_hWnd, GWL_STYLE, style);


    HMONITOR monitor = MonitorFromWindow(_hWnd, MONITOR_DEFAULTTONEAREST);
    MONITORINFO monitorInfo{ sizeof(monitorInfo) };

    if (GetMonitorInfoW(monitor, &monitorInfo))
    {
        const RECT& rc = fullScreen
            ? monitorInfo.rcMonitor
            : monitorInfo.rcWork;

        SetWindowPos(_hWnd, HWND_TOP, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, SWP_FRAMECHANGED);
    }
    else
    {
        SetWindowPos(_hWnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED);
    }
}

void Photino::SetIconFile(const AutoString filename)
{
    AutoString path = ToUTF16String(filename);
    _iconFileName = path;

    assert(_hWnd);
    if (!_hWnd) return;

    HICON iconSmall = static_cast<HICON>(LoadImageW(nullptr, path, IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_LOADFROMFILE | LR_SHARED));

    HICON iconBig = static_cast<HICON>(LoadImageW(nullptr, path, IMAGE_ICON, GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), LR_LOADFROMFILE | LR_SHARED));

    if (iconSmall)
        SendMessageW(_hWnd, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(iconSmall));

    if (iconBig)
        SendMessageW(_hWnd, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(iconBig));
}

void Photino::SetMinimized(const bool minimized)
{
    assert(_hWnd);
    if (!_hWnd) return;

    ShowWindow(_hWnd, minimized ? SW_MINIMIZE : SW_NORMAL);
}

void Photino::SetMinSize(const int width, const int height)
{
    _minWidth = width;
    _minHeight = height;

    assert(_hWnd);
    if (!_hWnd)  return;

    int currWidth = 0, currHeight = 0;
    GetSize(&currWidth, &currHeight);

    int newWidth = currWidth < _minWidth ? _minWidth : currWidth;
    int newHeight = currHeight < _minHeight ? _minHeight : currHeight;

    if (newWidth != currWidth || newHeight != currHeight)
        SetSize(newWidth, newHeight);
}

void Photino::SetMaximized(const bool maximized)
{
    assert(_hWnd);
    if (!_hWnd) return;

    ShowWindow(_hWnd, maximized ? SW_MAXIMIZE : SW_NORMAL);
}

void Photino::SetMaxSize(const int width, const int height)
{
    _maxWidth = width;
    _maxHeight = height;

    assert(_hWnd);
    if (!_hWnd) return;

    int currWidth = 0, currHeight = 0;
    GetSize(&currWidth, &currHeight);

    int newWidth = (currWidth > _maxWidth) ? _maxWidth : currWidth;
    int newHeight = (currHeight > _maxHeight) ? _maxHeight : currHeight;

    if (newWidth != currWidth || newHeight != currHeight)
        SetSize(newWidth, newHeight);
}

void Photino::SetPosition(const int x, const int y)
{
    assert(_hWnd);
    if (!_hWnd) return;

    SetWindowPos(_hWnd, nullptr, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
}

void Photino::SetResizable(const bool resizable)
{
    assert(_hWnd);
    if (!_hWnd)  return;

    LONG_PTR style = GetWindowLongPtr(_hWnd, GWL_STYLE);

    if (resizable)
        style |= (WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX);
    else
        style &= ~(WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX);

    SetWindowLongPtr(_hWnd, GWL_STYLE, style);
    // force non-client recalculation
    SetWindowPos(_hWnd, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
}

void Photino::SetSize(const int width, const int height)
{
    assert(_hWnd);
    if (!_hWnd)  return;

    SetWindowPos(_hWnd, nullptr, 0, 0, width, height, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
}

void Photino::SetTitle(AutoString title)
{
    title = ToUTF16String(title);
    if (wcslen(title) > 255)
    {
        for (int i = 0; i < 256; i++)
            _windowTitle[i] = title[i];
        _windowTitle[255] = 0;
    }
    else
        wcscpy(_windowTitle, title);
    SetWindowText(_hWnd, title);
    if (_notificationsEnabled)
    {
        WinToast::instance()->setAppName(title);
        if (_notificationRegistrationId == nullptr)
            WinToast::instance()->setAppUserModelId(title);
    }
}

void Photino::SetTopmost(const bool topmost)
{
    assert(_hWnd);
    if (!_hWnd) return;

    LONG_PTR exStyle = GetWindowLongPtr(_hWnd, GWL_EXSTYLE);

    if (topmost)    exStyle |= WS_EX_TOPMOST;
    else            exStyle &= ~WS_EX_TOPMOST;

    SetWindowLongPtr(_hWnd, GWL_EXSTYLE, exStyle);

    SetWindowPos(_hWnd, topmost ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
}

void Photino::SetZoom(int zoom)
{
    if (zoom < 25)          zoom = 25;
    else if (zoom > 500)    zoom = 500;

    _zoom = zoom;

    assert(_webviewController);
    if (!_webviewController) return;

    double newZoom = static_cast<double>(zoom) / 100.0;
    _webviewController->put_ZoomFactor(newZoom);
}

void Photino::ShowNotification(AutoString title, AutoString message)
{
    title = ToUTF16String(title);
    message = ToUTF16String(message);
    if (_notificationsEnabled && WinToast::isCompatible())
    {
        WinToastTemplate toast = WinToastTemplate(WinToastTemplate::ImageAndText02);
        toast.setTextField(title, WinToastTemplate::FirstLine);
        toast.setTextField(message, WinToastTemplate::SecondLine);
        if (this->_iconFileName != nullptr)
            toast.setImagePath(this->_iconFileName);
        WinToast::instance()->showToast(toast, _toastHandler);
    }
}

void Photino::WaitForExit() const
{
    assert(_hWnd);
    if (!_hWnd) return;

    messageLoopRootWindowHandle = _hWnd;

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

    messageLoopRootWindowHandle = nullptr;
}

//Callbacks
//https://learn.microsoft.com/en-us/windows/win32/api/winuser/nc-winuser-monitorenumproc
//To continue the enumeration, return TRUE.
//To stop the enumeration, return FALSE.
BOOL MonitorEnum(const HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, const LPARAM dwData)
{
    auto callback = reinterpret_cast<GetAllMonitorsCallback>(dwData);

    MONITORINFO info = {};
    info.cbSize = sizeof(info);
    if (!GetMonitorInfo(hMonitor, &info)) return TRUE;

    UINT dpiX = 96, dpiY = 96;
    if (FAILED(GetDpiForMonitor(hMonitor, MDT_EFFECTIVE_DPI, &dpiX, &dpiY)))
    {
        dpiX = dpiY = 96;
    }

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

void Photino::GetAllMonitors(GetAllMonitorsCallback callback)
{
    if (callback)
    {
        EnumDisplayMonitors(nullptr, nullptr, (MONITORENUMPROC)MonitorEnum, (LPARAM)callback);
    }
}

void Photino::Invoke(InvokeCallback callback) const
{
    InvokeWaitInfo waitInfo = {};
    PostMessage(_hWnd, WM_USER_INVOKE, (WPARAM)callback, (LPARAM)&waitInfo);

    // Block until the callback is actually executed and completed
    // TODO: Add return values, exception handling, etc.
    std::unique_lock<std::mutex> uLock(invokeLockMutex);
    waitInfo.completionNotifier.wait(uLock, [&] { return waitInfo.isCompleted; });
}





//private methods

AutoString Photino::ToUTF8String(const AutoString source)
{
    AutoString response;
    std::string* stringBuffer = new std::string();
    int inLen = (int)wcslen(source);
    int result = WideCharToMultiByte(CP_UTF8, 0, (LPCWCH)source, inLen, nullptr, 0, nullptr, nullptr);
    if (result < 0)
    {
        response = (AutoString)"UTF8 to UTF16 convert failed";
    }
    else
    {
        stringBuffer->resize(result, 0);
        result = WideCharToMultiByte(CP_UTF8, 0, (LPCWCH)source, inLen, &(*stringBuffer)[0], result, nullptr, nullptr);
        response = (AutoString)stringBuffer->c_str();
    }
    return response;
}
AutoString Photino::ToUTF16String(const AutoString source)
{
    AutoString response;
    std::wstring* wideBuffer = new std::wstring();
    int inLen = (int)strlen((char*)source);
    int result = MultiByteToWideChar(CP_UTF8, 0, (char*)source, inLen, nullptr, 0);
    if (result < 0)
    {
        response = (AutoString)"UTF8 to UTF16 convert failed";
    }
    else
    {
        wideBuffer->resize(result, 0);
        result = MultiByteToWideChar(CP_UTF8, 0, (char*)source, inLen, &(*wideBuffer)[0], result);
        response = (AutoString)wideBuffer->c_str();
    }
    return response;
}

void Photino::AttachWebView()
{
    size_t runtimePathLen = wcsnlen(_webview2RuntimePath, _countof(_webview2RuntimePath));
    PCWSTR runtimePath = runtimePathLen > 0 ? &_webview2RuntimePath[0] : nullptr;

    //TODO: Implement special startup strings.
    //https://peter.sh/experiments/chromium-command-line-switches/
    //https://learn.microsoft.com/en-us/dotnet/api/microsoft.web.webview2.core.corewebview2environmentoptions.additionalbrowserarguments?view=webview2-dotnet-1.0.1938.49&viewFallbackFrom=webview2-dotnet-1.0.1901.177view%3Dwebview2-1.0.1901.177
    //https://www.chromium.org/developers/how-tos/run-chromium-with-flags/
    //Add together all 7 special startup strings, plus the generic one passed by the user to make one big string. Try not to duplicate anything. Separate with spaces.

    std::wstring startupString = L"";
    if (_userAgent != nullptr && wcslen(_userAgent) > 0)
        startupString += L"--user-agent=\"" + std::wstring(_userAgent) + L"\" ";
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
    if (_browserControlInitParameters != nullptr)
        startupString += _browserControlInitParameters;	//e.g.--hide-scrollbars

    auto options = Microsoft::WRL::Make<CoreWebView2EnvironmentOptions>();
    if (startupString.length() > 0)
        options->put_AdditionalBrowserArguments(startupString.c_str());

    HRESULT envResult = CreateCoreWebView2EnvironmentWithOptions(runtimePath, _temporaryFilesPath, options.Get(),
        Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
            [&](const HRESULT result, ICoreWebView2Environment* env) -> HRESULT {
                if (result != S_OK) { return result; }
                HRESULT envResult = env->QueryInterface(&_webviewEnvironment);
                if (envResult != S_OK) { return envResult; }

                env->CreateCoreWebView2Controller(_hWnd, Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
                    [&](const HRESULT result, ICoreWebView2Controller* controller) -> HRESULT {

                        if (result != S_OK) { return result; }

                        HRESULT envResult = controller->QueryInterface(&_webviewController);
                        if (envResult != S_OK) { return envResult; }
                        _webviewController->get_CoreWebView2(&_webviewWindow);

                        ICoreWebView2Settings* Settings;
                        _webviewWindow->get_Settings(&Settings);
                        Settings->put_AreHostObjectsAllowed(TRUE);
                        Settings->put_IsScriptEnabled(TRUE);
                        Settings->put_AreDefaultScriptDialogsEnabled(TRUE);
                        Settings->put_IsWebMessageEnabled(TRUE);

                        EventRegistrationToken webMessageToken;
                        _webviewWindow->AddScriptToExecuteOnDocumentCreated(L"window.external = { sendMessage: function(message) { window.chrome.webview.postMessage(message); }, receiveMessage: function(callback) { window.chrome.webview.addEventListener(\'message\', function(e) { callback(e.data); }); } };", nullptr);
                        _webviewWindow->add_WebMessageReceived(Callback<ICoreWebView2WebMessageReceivedEventHandler>(
                            [&](ICoreWebView2* webview, ICoreWebView2WebMessageReceivedEventArgs* args) -> HRESULT {
                                wil::unique_cotaskmem_string message;
                                args->TryGetWebMessageAsString(&message);
                                _webMessageReceivedCallback(message.get());
                                return S_OK;
                            }).Get(), &webMessageToken);

                        EventRegistrationToken webResourceRequestedToken;
                        _webviewWindow->AddWebResourceRequestedFilter(L"*", COREWEBVIEW2_WEB_RESOURCE_CONTEXT_ALL);
                        _webviewWindow->add_WebResourceRequested(Callback<ICoreWebView2WebResourceRequestedEventHandler>(
                            [&](ICoreWebView2* sender, ICoreWebView2WebResourceRequestedEventArgs* args)
                            {
                                ICoreWebView2WebResourceRequest* req;
                                args->get_Request(&req);

                                wil::unique_cotaskmem_string uri;
                                req->get_Uri(&uri);
                                std::wstring uriString = uri.get();
                                size_t colonPos = uriString.find(L':', 0);
                                if (colonPos > 0)
                                {
                                    std::wstring scheme = uriString.substr(0, colonPos);
                                    std::vector<wchar_t*>::iterator it = std::find(_customSchemeNames.begin(), _customSchemeNames.end(), scheme);

                                    if (it != _customSchemeNames.end() && _customSchemeCallback != nullptr)
                                    {
                                        int numBytes;
                                        AutoString contentType;
                                        wil::unique_cotaskmem dotNetResponse(_customSchemeCallback((AutoString)uriString.c_str(), &numBytes, &contentType));

                                        if (dotNetResponse != nullptr && contentType != nullptr)
                                        {
                                            std::wstring contentTypeWS = contentType;

                                            IStream* dataStream = SHCreateMemStream((BYTE*)dotNetResponse.get(), numBytes);
                                            wil::com_ptr<ICoreWebView2WebResourceResponse> response;
                                            _webviewEnvironment->CreateWebResourceResponse(
                                                dataStream, 200, L"OK", (L"Content-Type: " + contentTypeWS).c_str(),
                                                &response);
                                            args->put_Response(response.get());
                                        }
                                    }
                                }

                                return S_OK;
                            }
                        ).Get(), &webResourceRequestedToken);

                        EventRegistrationToken permissionRequestedToken;
                        _webviewWindow->add_PermissionRequested(
                            Callback<ICoreWebView2PermissionRequestedEventHandler>(
                                [&](ICoreWebView2* sender, ICoreWebView2PermissionRequestedEventArgs* args)	-> HRESULT {
                                    if (_grantBrowserPermissions)
                                        args->put_State(COREWEBVIEW2_PERMISSION_STATE_ALLOW);
                                    return S_OK;
                                })
                            .Get(),
                            &permissionRequestedToken);

                        if (_startUrl != nullptr)
                            NavigateToUrl(_startUrl);
                        else if (_startString != nullptr)
                            NavigateToString(_startString);
                        else
                        {
                            MessageBox(nullptr, L"Neither StartUrl nor StartString was specified", L"Native Initialization Failed", MB_OK);
                            exit(0);
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
                    }).Get());
                return S_OK;
            }).Get());

    if (envResult != S_OK)
    {
        _com_error err(envResult);
        LPCTSTR errMsg = err.ErrorMessage();
        MessageBox(_hWnd, errMsg, L"Error instantiating webview", MB_OK);
    }
}


bool Photino::EnsureWebViewIsInstalled()
{
    LPWSTR* versionInfo = new wchar_t* [100];
    HRESULT ensureInstalledResult = GetAvailableCoreWebView2BrowserVersionString(nullptr, versionInfo);

    if (ensureInstalledResult != S_OK)
        return InstallWebView2();

    return true;
}

bool Photino::InstallWebView2()
{
    const wchar_t* srcURL = L"https://go.microsoft.com/fwlink/p/?LinkId=2124703";
    const wchar_t* destFile = L"MicrosoftEdgeWebview2Setup.exe";

    if (S_OK == URLDownloadToFile(nullptr, srcURL, destFile, 0, nullptr))
    {
        LPWSTR command = new wchar_t[100] { L"MicrosoftEdgeWebview2Setup.exe\0" };	//add these switches? /silent /install

        STARTUPINFO si;
        PROCESS_INFORMATION pi;

        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        ZeroMemory(&pi, sizeof(pi));

        bool success = CreateProcess(
            nullptr,		// No module name (use command line)
            command,	// Command line
            nullptr,       // Process handle not inheritable
            nullptr,       // Thread handle not inheritable
            FALSE,      // Set handle inheritance to FALSE
            0,          // No creation flags
            nullptr,       // Use parent's environment block
            nullptr,       // Use parent's starting directory
            &si,        // Pointer to STARTUPINFO structure
            &pi);		// Pointer to PROCESS_INFORMATION structure

        if (success)
        {
            // wait for the installation to complete
            WaitForSingleObject(pi.hProcess, INFINITE);
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
        }

        return success;
    }

    return false;
}

void Photino::RefitContent() const
{
    if (_webviewController)
    {
        RECT bounds;
        GetClientRect(_hWnd, &bounds);
        _webviewController->put_Bounds(bounds);
    }
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
    if (webview15)
    {
        wil::com_ptr<ICoreWebView2Profile> profile;
        webview15->get_Profile(&profile);
        auto profile2 = profile.try_query<ICoreWebView2Profile2>();

        if (profile2)
        {
            COREWEBVIEW2_BROWSING_DATA_KINDS dataKinds =
                (COREWEBVIEW2_BROWSING_DATA_KINDS)
                (COREWEBVIEW2_BROWSING_DATA_KINDS_GENERAL_AUTOFILL |
                    COREWEBVIEW2_BROWSING_DATA_KINDS_PASSWORD_AUTOSAVE);

            profile2->ClearBrowsingData(
                dataKinds,
                Callback<ICoreWebView2ClearBrowsingDataCompletedHandler>(
                    [this](HRESULT error)
                    -> HRESULT {
                        return S_OK;
                    })
                .Get());
        }
    }
}

void Photino::SetWebView2RuntimePath(const AutoString pathToWebView2)
{
    if (pathToWebView2 != nullptr)
    {
        wcsncpy(_webview2RuntimePath, pathToWebView2, _countof(_webview2RuntimePath));
    }
}

void Photino::Show(const bool isAlreadyShown)
{
    if (!isAlreadyShown)
        ShowWindow(_hWnd, SW_SHOWDEFAULT);	//causes maximized and minimized to not work

    UpdateWindow(_hWnd);

    // Strangely, it only works to create the webview2 *after* the window has been shown,
    // so defer it until here. This unfortunately means you can't call the Navigate methods
    // until the window is shown.
    if (!_webviewController)
    {
        if (wcsnlen(_webview2RuntimePath, _countof(_webview2RuntimePath)) > 0 || EnsureWebViewIsInstalled())
            AttachWebView();
        else
            exit(0);
    }
}
