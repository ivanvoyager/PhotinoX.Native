#pragma once

#ifdef _WIN32
#include <Windows.h>
#include <wil/com.h>
#include <WebView2.h>
typedef wchar_t* AutoString;
class WinToastHandler;
#else
// AutoString for macOS/Linux
typedef char* AutoString;
#endif

#ifdef __APPLE__
#include <Cocoa/Cocoa.h>
#include <Foundation/Foundation.h>
#include <UserNotifications/UserNotifications.h>
#include <WebKit/WebKit.h>
#include <WebKit/WKWebView.h>
#include <WebKit/WKWebViewConfiguration.h>
#include <Security/SecTrust.h>
#endif

#ifdef __linux__
#include <gtk/gtk.h>
#include <webkit2/webkit2.h>
#endif

#include <map>
#include <string>
#include <vector>

namespace PhotinoX::Native {

    struct Monitor
    {
        struct MonitorRect
        {
            int x, y;
            int width, height;
        } monitor, work;
        double scale;
    };

    using VoidCallback = void (*)();
    using BoolCallback = bool (*)();
    using IntIntCallback = void (*)(int, int);  // Resized, Moved
    using StringCallback = void (*)(AutoString);
    using ResourceCallback = void* (*)(AutoString, int*, AutoString*);
    using MonitorCallback = int (*)(const Monitor*);

    using InvokeCallback = VoidCallback;

    //no parameters, no return value
    using MaximizedCallback = VoidCallback;
    using RestoredCallback = VoidCallback;
    using MinimizedCallback = VoidCallback;
    using FocusInCallback = VoidCallback;
    using FocusOutCallback = VoidCallback;
    using ClosedCallback = VoidCallback;

    //with parameters and/or return value
    using ClosingCallback = BoolCallback;
    using ResizedCallback = IntIntCallback; //(int width, int height)
    using MovedCallback = IntIntCallback;   //(int x, int y)
    using WebMessageReceivedCallback = StringCallback;
    using WebResourceRequestedCallback = ResourceCallback;
    using GetAllMonitorsCallback = MonitorCallback;


    class PhotinoDialog;
    class Photino;

    struct PhotinoInitParams
    {
        AutoString StartString;//#1
        AutoString StartUrl;//#2
        AutoString Title;//#3
        AutoString WindowIconFile;//#4
        AutoString TemporaryFilesPath;//#5
        AutoString UserAgent;//#6
        AutoString BrowserControlInitParameters;//#7
        AutoString NotificationRegistrationId;//#8
        AutoString CustomSchemeNames[16];//#19

        Photino* ParentInstance;//#10

        ClosingCallback* ClosingHandler;//#11
        FocusInCallback* FocusInHandler;//#12
        FocusOutCallback* FocusOutHandler;//#13
        ResizedCallback* ResizedHandler;//#14
        MaximizedCallback* MaximizedHandler;//#15
        RestoredCallback* RestoredHandler;//#16
        MinimizedCallback* MinimizedHandler;//#17
        MovedCallback* MovedHandler;//#18
        WebMessageReceivedCallback* WebMessageReceivedHandler;//#19	
        WebResourceRequestedCallback* CustomSchemeHandler;//#20
        ClosedCallback* ClosedHandler;//#21

        int Left;//#22
        int Top;//#23
        int Width;//#24
        int Height;//#25
        int Zoom;//#26
        int MinWidth;//#27
        int MinHeight;//#28
        int MaxWidth;//#29
        int MaxHeight;//#30

        bool CenterOnInitialize;//#31
        bool Chromeless;//#32
        bool Transparent;//#33
        bool ContextMenuEnabled;//#34
        bool ZoomEnabled;//#35
        bool DevToolsEnabled;//#36
        bool FullScreen;//#37
        bool Maximized;//#38
        bool Minimized;//#39
        bool Resizable;//#40
        bool Topmost;//#41
        bool UseOsDefaultLocation;//#42
        bool UseOsDefaultSize;//#43
        bool GrantBrowserPermissions;//#44
        bool MediaAutoplayEnabled;//#45
        bool FileSystemAccessEnabled;//#46
        bool WebSecurityEnabled;//#47
        bool JavascriptClipboardAccessEnabled;//#48
        bool MediaStreamEnabled;//#49
        bool SmoothScrollingEnabled;//#50
        bool IgnoreCertificateErrorsEnabled;//#51
        bool NotificationsEnabled;//#52

        int Size;//#53
    };

    class Photino
    {
    private:
        WebMessageReceivedCallback _webMessageReceivedCallback;
        MovedCallback _movedCallback;
        ResizedCallback _resizedCallback;
        MaximizedCallback _maximizedCallback;
        RestoredCallback _restoredCallback;
        MinimizedCallback _minimizedCallback;
        ClosingCallback _closingCallback;
        ClosedCallback _closedCallback;
        FocusInCallback _focusInCallback;
        FocusOutCallback _focusOutCallback;
        std::vector<AutoString> _customSchemeNames;
        WebResourceRequestedCallback _customSchemeCallback;

        AutoString _startUrl;
        AutoString _startString;
        AutoString _temporaryFilesPath;
        AutoString _windowTitle;
        AutoString _iconFileName;
        AutoString _userAgent;
        AutoString _browserControlInitParameters;
        AutoString _notificationRegistrationId;

        bool _transparentEnabled;
        bool _devToolsEnabled;
        bool _grantBrowserPermissions;
        bool _mediaAutoplayEnabled;
        bool _fileSystemAccessEnabled;
        bool _webSecurityEnabled;
        bool _javascriptClipboardAccessEnabled;
        bool _mediaStreamEnabled;
        bool _smoothScrollingEnabled;
        bool _ignoreCertificateErrorsEnabled;
        bool _notificationsEnabled;
        bool _contextMenuEnabled;
        bool _zoomEnabled;
        bool isClosing_ = false;

        int _zoom;

        Photino* _parent;
        PhotinoDialog* _dialog;
        void Show(bool isAlreadyShown);
#ifdef _WIN32
        HWND _hWnd;
        WinToastHandler* _toastHandler;
        wil::com_ptr<ICoreWebView2Environment> _webviewEnvironment;
        wil::com_ptr<ICoreWebView2> _webviewWindow;
        wil::com_ptr<ICoreWebView2Controller> _webviewController;
        bool EnsureWebViewIsInstalled();
        bool InstallWebView2();
        void AttachWebView();

#elif __linux__
        // GtkWidget* _window;
        GtkWidget* _webview;
        GdkGeometry _hints;
        void AddCustomSchemeHandlers();
        bool _isFullScreen;
#elif __APPLE__
        NSWindow* _window;
        WKWebView* _webview;
        WKWebViewConfiguration* _webviewConfiguration;
        std::vector<Monitor*> GetMonitors() const;

        bool _chromeless;

        int _preMaximizedWidth;
        int _preMaximizedHeight;
        int _preMaximizedXPosition;
        int _preMaximizedYPosition;

        void AttachWebView();
        void AddCustomScheme(AutoString scheme, WebResourceRequestedCallback requestHandler);

        void SetUserAgent(AutoString userAgent);

        void SetPreference(NSString* key, NSNumber* value);
        void SetPreference(NSString* key, NSString* value);
#endif
    public:
#ifdef _WIN32
        static void Register(HINSTANCE hInstance);
        static void SetWebView2RuntimePath(AutoString pathToWebView2);
        HWND getHwnd() const;
        void RefitContent() const;
        void FocusWebView2() const;
        void NotifyWebView2WindowMove() const;
        void GetNotificationsEnabled(bool* enabled) const;
        AutoString ToUTF16String(AutoString source);
        AutoString ToUTF8String(AutoString source);
        int _minWidth;
        int _minHeight;
        int _maxWidth;
        int _maxHeight;
#elif __linux__
        static void Register();
        void set_webkit_settings();
        void set_webkit_custom_settings(WebKitSettings* settings);
        GtkWidget* _window;
        int _lastHeight;
        int _lastWidth;
        int _lastTop;
        int _lastLeft;
        int _minWidth;
        int _minHeight;
        int _maxWidth;
        int _maxHeight;
#elif __APPLE__
        static void Register();
#endif

        Photino(PhotinoInitParams* initParams);
        ~Photino();

        PhotinoDialog* GetDialog() const { return _dialog; };

        void Center();
        void ClearBrowserAutoFill() const;
        void Close() const;

        void GetTransparentEnabled(bool* enabled) const;
        void GetContextMenuEnabled(bool* enabled) const;
        void GetZoomEnabled(bool* enabled) const;
        void GetDevToolsEnabled(bool* enabled) const;
        void GetFullScreen(bool* fullScreen) const;
        void GetGrantBrowserPermissions(bool* grant) const;
        AutoString GetUserAgent() const;
        void GetMediaAutoplayEnabled(bool* enabled) const;
        void GetFileSystemAccessEnabled(bool* enabled) const;
        void GetWebSecurityEnabled(bool* enabled) const;
        void GetJavascriptClipboardAccessEnabled(bool* enabled) const;
        void GetMediaStreamEnabled(bool* enabled) const;
        void GetSmoothScrollingEnabled(bool* enabled) const;
        AutoString GetIconFileName() const;
        void GetMaximized(bool* isMaximized) const;
        void GetMinimized(bool* isMinimized) const;
        void GetPosition(int* x, int* y) const;
        void GetResizable(bool* resizable) const;
        unsigned int GetScreenDpi() const;
        void GetSize(int* width, int* height) const;
        AutoString GetTitle() const;
        void GetTopmost(bool* topmost) const;
        void GetZoom(int* zoom) const;
        void GetIgnoreCertificateErrorsEnabled(bool* enabled) const;

        void NavigateToString(AutoString content);
        void NavigateToUrl(AutoString url);
        void Restore(); // required anymore?backward compat?
        void SendWebMessage(AutoString message);

        void SetTransparentEnabled(bool enabled);
        void SetContextMenuEnabled(bool enabled);
        void SetZoomEnabled(bool enabled);
        void SetDevToolsEnabled(bool enabled);
        void SetIconFile(AutoString filename);
        void SetFullScreen(bool fullScreen);
        void SetMaximized(bool maximized);
        void SetMaxSize(int width, int height);
        void SetMinimized(bool minimized);
        void SetMinSize(int width, int height);
        void SetPosition(int x, int y);
        void SetResizable(bool resizable);
        void SetSize(int width, int height);
        void SetTitle(AutoString title);
        void SetTopmost(bool topmost);
        void SetZoom(int zoom);

        void ShowNotification(AutoString title, AutoString message);
        void WaitForExit() const;
        void CloseWebView();

        // Callbacks
        void AddCustomSchemeName(AutoString scheme) { _customSchemeNames.push_back((AutoString)scheme); };
        void GetAllMonitors(GetAllMonitorsCallback callback);
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

        bool InvokeClosing()
        {
            if (!_closingCallback || isClosing_) return false;

            isClosing_ = true;
            bool result = _closingCallback();
            isClosing_ = false;

            return result;
        }

        void InvokeClose()
        {
            if (_closedCallback) _closedCallback();
        }

        void InvokeFocusIn()
        {
            if (_focusInCallback) _focusInCallback();
        }

        void InvokeFocusOut()
        {
            if (_focusOutCallback) _focusOutCallback();
        }

        void InvokeMove(int x, int y)
        {
            if (_movedCallback) _movedCallback(x, y);
        }

        void InvokeResize(int width, int height)
        {
            if (_resizedCallback) _resizedCallback(width, height);
        }

        void InvokeMaximized()
        {
            if (_maximizedCallback) _maximizedCallback();
        }

        void InvokeRestored()
        {
            if (_restoredCallback) _restoredCallback();
        }

        void InvokeMinimized()
        {
            if (_minimizedCallback) _minimizedCallback();
        }
    };

} // namespace PhotinoX::Native