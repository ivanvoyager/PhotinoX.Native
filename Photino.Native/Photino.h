#pragma once

#include "Photino.Callbacks.h"
#include "Photino.InitParams.h"
#include "Photino.Strings.h"

#ifndef _WIN32
#include <strings.h>
#endif
#include <utility>
#include <vector>

#ifdef _WIN32
#include <WebView2.h>
#include <Windows.h>
#include <wil/com.h>
class WinToastHandler;
#endif

#ifdef __APPLE__
#include <Cocoa/Cocoa.h>
#include <Foundation/Foundation.h>
#include <UserNotifications/UserNotifications.h>
#include <WebKit/WebKit.h>
#include <WebKit/WKWebView.h>
#include <WebKit/WKWebViewConfiguration.h>
#include <Security/SecTrust.h>

@class WindowDelegate;
@class UiDelegate;
@class NavigationDelegate;
#endif

#ifdef __linux__
#include <gtk/gtk.h>
#include <webkit2/webkit2.h>
#endif

namespace PhotinoX::Native 
{
    struct WindowSizeLimits
    {
        int minWidth = 0;
        int minHeight = 0;
        int maxWidth = 0;
        int maxHeight = 0;
    };

    struct WindowGeometry
    {
        int left = 0;
        int top = 0;
        int width = 0;
        int height = 0;
    };

    struct Monitor;
    class PhotinoDialog;

    class Photino
    {
    private:
        WebMessageReceivedCallback _webMessageReceivedCallback = nullptr;
        MovedCallback _movedCallback = nullptr;
        ResizedCallback _resizedCallback = nullptr;
        MaximizedCallback _maximizedCallback = nullptr;
        RestoredCallback _restoredCallback = nullptr;
        MinimizedCallback _minimizedCallback = nullptr;
        ClosingCallback _closingCallback = nullptr;
        ClosedCallback _closedCallback = nullptr;
        FocusInCallback _focusInCallback = nullptr;
        FocusOutCallback _focusOutCallback = nullptr;
        std::vector<PlatformString> _customSchemeNames;
        WebResourceRequestedCallback _customSchemeCallback = nullptr;

        PlatformString _startUrl;
        PlatformString _startString;
        PlatformString _temporaryFilesPath;
        PlatformString _windowTitle;
        PlatformString _iconFileName;
        PlatformString _userAgent;
        PlatformString _browserControlInitParameters;
        PlatformString _notificationRegistrationId;

        bool _transparentEnabled;
        bool _devToolsEnabled;
        bool _grantBrowserPermissions;
#if defined(_WIN32) || defined(__linux__)
        bool _mediaAutoplayEnabled;
#endif
        bool _fileSystemAccessEnabled;
        bool _webSecurityEnabled;
        bool _javascriptClipboardAccessEnabled;
        bool _mediaStreamEnabled;
#if defined(_WIN32) || defined(__linux__)
        bool _smoothScrollingEnabled;
#endif
        bool _ignoreCertificateErrorsEnabled;
        bool _notificationsEnabled;
        bool _contextMenuEnabled;
        bool _zoomEnabled;
        mutable bool _isClosing = false;

        int _zoom;
        bool _chromeless;
        bool _fullScreen;

        Photino* _parent = nullptr;
        PhotinoDialog* _dialog = nullptr;
        void Show();

        bool IsCustomScheme(const PlatformString& scheme) const
        {
            if (scheme.empty())
                return false;

#ifdef _WIN32
            for (const auto& existing : _customSchemeNames)
            {
                if (_wcsicmp(existing.c_str(), scheme.c_str()) == 0)
                    return true;
            }
#else
            for (const auto& existing : _customSchemeNames)
            {
                if (strcasecmp(existing.c_str(), scheme.c_str()) == 0)
                    return true;
            }
#endif
            return false;
        }

        bool RegisterCustomSchemeName(const PlatformString& scheme);

#ifdef _WIN32
        HWND _hWnd = nullptr;
        WinToastHandler* _toastHandler = nullptr;
        wil::com_ptr<ICoreWebView2Environment> _webviewEnvironment = nullptr;
        wil::com_ptr<ICoreWebView2Controller> _webviewController = nullptr;
        wil::com_ptr<ICoreWebView2> _webviewWindow = nullptr;
        PlatformString _scriptId;
        WindowSizeLimits _sizeLimits;
        bool _webViewInitialized = false;
        bool _isAlreadyShown = false;

        PlatformString BuildStartupString() const;
        HRESULT CompleteWebViewInitialization();
        HRESULT HandleScriptAddedOnDocumentCreated(HRESULT result, LPCWSTR id);
        HRESULT HandleWebMessageReceived(ICoreWebView2* webview, ICoreWebView2WebMessageReceivedEventArgs* args);
        HRESULT HandleWebResourceRequested(ICoreWebView2* webview, ICoreWebView2WebResourceRequestedEventArgs* args);
        HRESULT HandlePermissionRequested(ICoreWebView2* webview, ICoreWebView2PermissionRequestedEventArgs* args);
        HRESULT HandleWebViewControllerCreated(HRESULT result, ICoreWebView2Controller* controller);
        HRESULT HandleWebViewEnvironmentCreated(HRESULT result, ICoreWebView2Environment* environment);
        void AttachWebView();

        static bool EnsureWebViewIsInstalled();
        static bool InstallWebView2();
        void NotifyWebView2WindowMove() const;

#elif defined(__linux__)
        GtkWidget* _window = nullptr;
        GtkWidget* _webview = nullptr;
        GdkGeometry _hints{};
        WindowGeometry _lastGeometry;
        WindowSizeLimits _sizeLimits;
        bool _notifyInitialized = false;

        void ApplyGeometryHints();
        void AddCustomSchemeHandlers();
        void SetWebKitSettings();
        void SetWebKitCustomSettings(WebKitSettings* settings);

#elif defined(__APPLE__)
        NSWindow* _window = nullptr;
        WKWebView* _webview = nullptr;
        WKWebViewConfiguration* _webviewConfiguration = nullptr;

        WindowDelegate* _windowDelegate = nullptr;
        UiDelegate* _uiDelegate = nullptr;
        NavigationDelegate* _navigationDelegate = nullptr;

        std::vector<Monitor> GetMonitors() const;

        void AttachWebView();
        void AddCustomSchemeHandlers();

        void SetUserAgent(const PlatformString& userAgent);

        bool SetPreference(NSString* key, NSNumber* value);
        bool SetPreference(NSString* key, NSString* value);
#endif
    public:
#ifdef _WIN32

        static void Register(HINSTANCE hInstance);
        static void SetWebView2RuntimePath(const PlatformString& pathToWebView2);
        HWND GetHwnd() const noexcept
        {
            return _hWnd;
        }
        void ApplySizeLimits(MINMAXINFO& info) const noexcept;
        void FocusWebView2() const;
        void RefitContent() const;
        void CloseWebView();

#elif defined(__linux__)

        static void Register();
        void HandleConfigureEvent(int x, int y, int width, int height);
        void* GetGtkWidget() const noexcept
        {
            return _window;
        }

#elif defined(__APPLE__) && defined(__OBJC__)

        static void Register();
        void* GetNSWindow() const noexcept
        {
            return (__bridge void*)_window;
        }

#endif

        Photino(PhotinoInitParams* initParams);
        ~Photino();

        PhotinoDialog* GetDialog() const noexcept
        {
            return _dialog;
        }

        void Center();
        void ClearBrowserAutoFill() const;
        void Close() const;

        void GetTransparentEnabled(bool* enabled) const;
        void GetContextMenuEnabled(bool* enabled) const;
        void GetZoomEnabled(bool* enabled) const;
        void GetDevToolsEnabled(bool* enabled) const;
        void GetFullScreen(bool* fullScreen) const;
        void GetGrantBrowserPermissions(bool* grant) const;
        const PlatformString& GetUserAgent() const { return _userAgent; }
        void GetMediaAutoplayEnabled(bool* enabled) const;
        void GetFileSystemAccessEnabled(bool* enabled) const;
        void GetWebSecurityEnabled(bool* enabled) const;
        void GetJavascriptClipboardAccessEnabled(bool* enabled) const;
        void GetMediaStreamEnabled(bool* enabled) const;
        void GetSmoothScrollingEnabled(bool* enabled) const;
        const PlatformString& GetIconFile() const noexcept
        {
            return _iconFileName;
        }
        void GetMaximized(bool* isMaximized) const;
        void GetMinimized(bool* isMinimized) const;
        void GetPosition(int* x, int* y) const;
        void GetResizable(bool* resizable) const;
        unsigned int GetScreenDpi() const;
        void GetSize(int* width, int* height) const;
        const PlatformString& GetTitle() const { return _windowTitle; }
        void GetTopmost(bool* topmost) const;
        void GetZoom(int* zoom) const;
        void GetIgnoreCertificateErrorsEnabled(bool* enabled) const;
        void GetNotificationsEnabled(bool* enabled) const;

        void NavigateToString(const PlatformString& content) const;
        void NavigateToUrl(const PlatformString& url) const;
        void Restore() const;
        void SendWebMessage(const PlatformString& message) const;

        void SetTransparentEnabled(bool enabled);
        void SetContextMenuEnabled(bool enabled);
        void SetZoomEnabled(bool enabled);
        void SetDevToolsEnabled(bool enabled);
        void SetIconFile(const PlatformString& filename);
        void SetFullScreen(bool fullScreen);
        void SetMaximized(bool maximized);
        void SetMaxSize(int width, int height);
        void SetMinimized(bool minimized);
        void SetMinSize(int width, int height);
        void SetPosition(int x, int y);
        void SetResizable(bool resizable);
        void SetSize(int width, int height);
        void SetTitle(const PlatformString& title);
        void SetTopmost(bool topmost);
        void SetZoom(int zoom);

        void ShowNotification(const PlatformString& title, const PlatformString& message) const;
        void WaitForExit() const;

        // Callbacks
        bool AddCustomSchemeName(Utf8String scheme)
        {
            if (!scheme || *scheme == '\0') return false;

            PlatformString nativeScheme = ToPlatformString(scheme);
            if (nativeScheme.empty()) return false;

            if (IsCustomScheme(nativeScheme)) return true;

            if (_customSchemeNames.size() >= MaxCustomSchemeNames) return false;

            _customSchemeNames.emplace_back(std::move(nativeScheme));

            return RegisterCustomSchemeName(_customSchemeNames.back());
        }

        void GetAllMonitors(GetAllMonitorsCallback callback) const;
        void SetClosingCallback(ClosingCallback callback) { _closingCallback = callback; }
        void SetClosedCallback(ClosedCallback callback) { _closedCallback = callback; }
        void SetFocusInCallback(FocusInCallback callback) { _focusInCallback = callback; }
        void SetFocusOutCallback(FocusOutCallback callback) { _focusOutCallback = callback; }
        void SetMovedCallback(MovedCallback callback) { _movedCallback = callback; }
        void SetResizedCallback(ResizedCallback callback) { _resizedCallback = callback; }
        void SetMaximizedCallback(MaximizedCallback callback) { _maximizedCallback = callback; }
        void SetRestoredCallback(RestoredCallback callback) { _restoredCallback = callback; }
        void SetMinimizedCallback(MinimizedCallback callback) { _minimizedCallback = callback; }

        void Invoke(InvokeCallback callback) const;

        bool InvokeClosing() const
        {
            if (!_closingCallback || _isClosing)
                return false;

            _isClosing = true;
            bool result = _closingCallback();
            _isClosing = false;

            return result;
        }

        void InvokeClose() const
        {
            if (_closedCallback) _closedCallback();
        }

        void InvokeFocusIn() const
        {
            if (_focusInCallback) _focusInCallback();
        }

        void InvokeFocusOut() const
        {
            if (_focusOutCallback) _focusOutCallback();
        }

        void InvokeMove(int x, int y) const
        {
            if (_movedCallback) _movedCallback(x, y);
        }

        void InvokeResize(int width, int height) const
        {
            if (_resizedCallback) _resizedCallback(width, height);
        }

        void InvokeMaximized() const
        {
            if (_maximizedCallback) _maximizedCallback();
        }

        void InvokeRestored() const
        {
            if (_restoredCallback) _restoredCallback();
        }

        void InvokeMinimized() const
        {
            if (_minimizedCallback) _minimizedCallback();
        }
    };

} // namespace PhotinoX::Native