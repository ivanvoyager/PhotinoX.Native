#pragma once

#include "Photino.Callbacks.h"
#include "Photino.InitParams.h"
#include "Photino.Strings.h"
#include "Photino.Monitor.h"

#include <memory>
#include <vector>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>

struct ICoreWebView2;
struct ICoreWebView2Environment;
struct ICoreWebView2Controller;
struct ICoreWebView2WebMessageReceivedEventArgs;
struct ICoreWebView2WebResourceRequestedEventArgs;
struct ICoreWebView2PermissionRequestedEventArgs;
#endif

namespace PhotinoX::Native 
{
#ifdef _WIN32
    struct WindowsState;
#elif defined(__linux__)
    struct LinuxState;
#elif defined(__APPLE__)
    struct MacState;
#endif

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

        // Window metadata
        PlatformString _windowTitle;
        PlatformString _iconFileName;

        PlatformString _startUrl;
        PlatformString _startString;
        PlatformString _temporaryFilesPath;
        PlatformString _userAgent;
        PlatformString _browserControlInitParameters;
        PlatformString _notificationRegistrationId;

        bool _transparentEnabled = false;
        bool _devToolsEnabled = true;
        bool _grantBrowserPermissions = true;
#if defined(_WIN32) || defined(__linux__)
        bool _mediaAutoplayEnabled = true;
#endif
        bool _fileSystemAccessEnabled = true;
        bool _webSecurityEnabled = true;
        bool _javascriptClipboardAccessEnabled = true;
        bool _mediaStreamEnabled = true;
#if defined(_WIN32) || defined(__linux__)
        bool _smoothScrollingEnabled = true;
#endif
        bool _ignoreCertificateErrorsEnabled = false;
        bool _notificationsEnabled = true;
        bool _contextMenuEnabled = true;
        bool _zoomEnabled = true;
        mutable bool _isClosing = false;

        int _zoom = 100;
        bool _chromeless = false;
        bool _fullScreen = false;

        Photino* _parent = nullptr;
        PhotinoDialog* _dialog = nullptr;

#ifdef _WIN32
        std::unique_ptr<WindowsState> platform_;
#elif defined(__linux__)
        std::unique_ptr<LinuxState> platform_;
#elif defined(__APPLE__)
        std::unique_ptr<MacState> platform_;
#endif

        void Show();

        bool IsCustomSchemeRegistered(const PlatformString& scheme) const;

        bool RegisterCustomSchemeName(const PlatformString& scheme);

#ifdef _WIN32
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
        void ApplyGeometryHints();

        void AddCustomSchemeHandlers();
        void SetWebKitSettings();
#elif defined(__APPLE__)
        std::vector<Monitor> GetMonitors() const;

        void AttachWebView();
        void AddCustomSchemeHandlers();

        void SetUserAgent(const PlatformString& userAgent);
        void ConfigureWebViewPreferences(const PhotinoInitParams* initParams);
#endif
    public:
#ifdef _WIN32
        void ApplySizeLimits(MINMAXINFO& info) const noexcept;
        void RefitContent() const;
        void FocusWebView2() const;
        void CloseWebView();
#elif defined(__linux__)
        void HandleConfigureEvent(int x, int y, int width, int height);
#endif
        Photino(PhotinoInitParams* initParams);
        ~Photino();
#ifdef _WIN32
        static void Register(HINSTANCE hInstance);
        static void SetWebView2RuntimePath(const PlatformString& pathToWebView2);
#elif defined(__linux__)
        static void Register();
#elif defined(__APPLE__)
        static void Register();
#endif
        // Platform handles
#ifdef _WIN32
        HWND GetHwnd() const noexcept;
#elif defined(__linux__)
        void* GetGtkWidget() const noexcept;
#elif defined(__APPLE__)
        void* GetNSWindow() const noexcept;
#endif
        void Close() const;

        // Window metadata
        const PlatformString& GetTitle() const noexcept { return _windowTitle; }
        void SetTitle(const PlatformString& title);

        const PlatformString& GetIconFile() const noexcept { return _iconFileName; }
        void SetIconFile(const PlatformString& filename);

        // Window geometry
        void GetPosition(int* x, int* y) const;
        void SetPosition(int x, int y);

        void GetSize(int* width, int* height) const;
        void SetSize(int width, int height);

        void SetMinSize(int width, int height);
        void SetMaxSize(int width, int height);

        void Center() const;
        void Restore() const;

        unsigned int GetScreenDpi() const;
        void GetAllMonitors(GetAllMonitorsCallback callback) const noexcept;

        // Window state
        void GetFullScreen(bool* fullScreen) const;
        void SetFullScreen(bool fullScreen);

        void GetMaximized(bool* isMaximized) const;
        void SetMaximized(bool maximized);

        void GetMinimized(bool* isMinimized) const;
        void SetMinimized(bool minimized);

        void GetResizable(bool* resizable) const;
        void SetResizable(bool resizable);

        void GetTopmost(bool* topmost) const;
        void SetTopmost(bool topmost);

        // Misc
        PhotinoDialog* GetDialog() const noexcept { return _dialog; }

        // Browser / Navigation / Messaging
        void GetTransparentEnabled(bool* enabled) const;
        void SetTransparentEnabled(bool enabled);

        void ClearBrowserAutoFill() const;

        void NavigateToString(const PlatformString& content) const;
        void NavigateToUrl(const PlatformString& url) const;

        void SendWebMessage(const PlatformString& message) const;

        bool AddCustomSchemeName(Utf8String scheme);

        void GetContextMenuEnabled(bool* enabled) const;
        void SetContextMenuEnabled(bool enabled);

        void GetZoomEnabled(bool* enabled) const;
        void SetZoomEnabled(bool enabled);

        void GetZoom(int* zoom) const;
        void SetZoom(int zoom);

        void GetDevToolsEnabled(bool* enabled) const;
        void SetDevToolsEnabled(bool enabled);
        
        void GetGrantBrowserPermissions(bool* grant) const;
        const PlatformString& GetUserAgent() const noexcept { return _userAgent; }
        void GetMediaAutoplayEnabled(bool* enabled) const;
        void GetFileSystemAccessEnabled(bool* enabled) const;
        void GetWebSecurityEnabled(bool* enabled) const;
        void GetJavascriptClipboardAccessEnabled(bool* enabled) const;
        void GetMediaStreamEnabled(bool* enabled) const;
        void GetSmoothScrollingEnabled(bool* enabled) const;
        void GetIgnoreCertificateErrorsEnabled(bool* enabled) const;

        // App
        void GetNotificationsEnabled(bool* enabled) const;
        void ShowNotification(const PlatformString& title, const PlatformString& message) const;
        void WaitForExit() const;

        // Callbacks
        void SetClosingCallback(ClosingCallback callback) noexcept { _closingCallback = callback; }
        void SetClosedCallback(ClosedCallback callback) noexcept { _closedCallback = callback; }
        void SetFocusInCallback(FocusInCallback callback) noexcept { _focusInCallback = callback; }
        void SetFocusOutCallback(FocusOutCallback callback) noexcept { _focusOutCallback = callback; }
        void SetMovedCallback(MovedCallback callback) noexcept { _movedCallback = callback; }
        void SetResizedCallback(ResizedCallback callback) noexcept { _resizedCallback = callback; }
        void SetMaximizedCallback(MaximizedCallback callback) noexcept { _maximizedCallback = callback; }
        void SetRestoredCallback(RestoredCallback callback) noexcept { _restoredCallback = callback; }
        void SetMinimizedCallback(MinimizedCallback callback) noexcept { _minimizedCallback = callback; }

        void Invoke(InvokeCallback callback) const;

        bool InvokeClosing() const noexcept;
        void InvokeClose() const noexcept;
        void InvokeFocusIn() const noexcept;
        void InvokeFocusOut() const noexcept;
        void InvokeMove(int x, int y) const noexcept;
        void InvokeResize(int width, int height) const noexcept;
        void InvokeMaximized() const noexcept;
        void InvokeRestored() const noexcept;
        void InvokeMinimized() const noexcept;
    };

} // namespace PhotinoX::Native