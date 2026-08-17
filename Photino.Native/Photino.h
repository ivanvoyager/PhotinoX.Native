#pragma once

#include "Photino.Enums.h"
#include "Photino.Callbacks.h"
#include "Photino.InitParams.h"
#include "Photino.Options.h"
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
struct ICoreWebView2EnvironmentOptions;
struct ICoreWebView2Controller;
struct ICoreWebView2WebMessageReceivedEventArgs;
struct ICoreWebView2ContentLoadingEventArgs;
struct ICoreWebView2NavigationCompletedEventArgs;
struct ICoreWebView2NavigationStartingEventArgs;
struct ICoreWebView2NewWindowRequestedEventArgs;
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
        WebMessageReceivedCallback webMessageReceivedCallback_ = nullptr;
        ContentLoadingCallback contentLoadingCallback_ = nullptr;
        ContentLoadedCallback contentLoadedCallback_ = nullptr;
        NavigationStartingCallback navigationStartingCallback_ = nullptr;
        NewWindowRequestedCallback newWindowRequestedCallback_ = nullptr;
        MovedCallback movedCallback_ = nullptr;
        ResizedCallback resizedCallback_ = nullptr;
        MaximizedCallback maximizedCallback_ = nullptr;
        RestoredCallback restoredCallback_ = nullptr;
        MinimizedCallback minimizedCallback_ = nullptr;
        ClosingCallback closingCallback_ = nullptr;
        ClosedCallback closedCallback_ = nullptr;
        FocusInCallback focusInCallback_ = nullptr;
        FocusOutCallback focusOutCallback_ = nullptr;
        FullScreenChangedCallback fullScreenChangedCallback_ = nullptr;
        StateChangedCallback stateChangedCallback_ = nullptr;
        WebResourceRequestedCallback customSchemeCallback_ = nullptr;
        std::vector<PlatformString> customSchemeNames_;

        PhotinoOptions options_;
        Photino* parent_ = nullptr;
        PhotinoDialog* dialog_ = nullptr;

        bool suppressWindowStateCallbacks_ = false;
        bool suppressRestoredCallback_ = false;

#ifdef _WIN32
        std::unique_ptr<WindowsState> platform_;
#elif defined(__linux__)
        std::unique_ptr<LinuxState> platform_;
#elif defined(__APPLE__)
        std::unique_ptr<MacState> platform_;
#endif
        mutable bool isClosing_ = false;

        void InitializeFromInitParams(const PhotinoInitParams* initParams);
        void InitializeOptions(const PhotinoInitParams* initParams);
        void InitializeCallbacks(const PhotinoInitParams* initParams);
        void InitializeCustomSchemes(const PhotinoInitParams* initParams);

        bool IsCustomSchemeRegistered(const PlatformString& scheme) const;
        bool RegisterCustomSchemeName(const PlatformString& scheme);

        // Common state
        PhotinoWindowState GetPlatformWindowState() const noexcept;
        bool ChangeWindowState(PhotinoWindowState state) noexcept;

#ifdef _WIN32
        PlatformString BuildStartupString() const;
        HRESULT CompleteWebViewInitialization();
        HRESULT HandleScriptAddedOnDocumentCreated(HRESULT result, LPCWSTR id);
        HRESULT HandleWebMessageReceived(ICoreWebView2* webview, ICoreWebView2WebMessageReceivedEventArgs* args);
        HRESULT HandleNavigationStarting(ICoreWebView2* webview, ICoreWebView2NavigationStartingEventArgs* args);
        HRESULT HandleNewWindowRequested(ICoreWebView2* webview, ICoreWebView2NewWindowRequestedEventArgs* args);
        HRESULT HandleContentLoading(ICoreWebView2* webview, ICoreWebView2ContentLoadingEventArgs* args);
        HRESULT HandleNavigationCompleted(ICoreWebView2* webview, ICoreWebView2NavigationCompletedEventArgs* args);
        HRESULT HandleWebResourceRequested(ICoreWebView2* webview, ICoreWebView2WebResourceRequestedEventArgs* args);
        HRESULT HandlePermissionRequested(ICoreWebView2* webview, ICoreWebView2PermissionRequestedEventArgs* args);
        HRESULT HandleWebViewControllerCreated(HRESULT result, ICoreWebView2Controller* controller);
        HRESULT HandleWebViewEnvironmentCreated(HRESULT result, ICoreWebView2Environment* environment);
        HRESULT ConfigureCustomSchemeRegistrations(ICoreWebView2EnvironmentOptions* options) const;

        void AttachWebView();
        bool EnsureWebViewAttached();
        static bool EnsureWebViewIsInstalled();
        static bool InstallWebView2();
        void NotifyWebView2WindowMove() const;
        void ReloadWebView() const;

        bool SaveFullScreenRestoreState();
        bool RestoreFullScreenRestoreState();
        void ResetFullScreenRestoreState();
        bool SkipFullScreenChange(bool fullScreen) const noexcept;
        bool EnterFullScreen();
        bool ExitFullScreen();
        bool ShowWindowAfterFullScreenExit(int showCommand);

#elif defined(__linux__)
        void ApplyGeometryHints();

        void ApplyPendingStateAfterFullScreenExit();
        void CompleteFullScreenTransition();

        void SaveNormalGeometry();
        void RestoreNormalGeometry();
        void ScheduleRestoreNormalGeometry();

        bool EnsureWebViewAttached();
        void AddCustomSchemeHandlers();
        void SetWebKitSettings();
#elif defined(__APPLE__)
        std::vector<Monitor> GetMonitors() const;

        void ApplyPendingStateAfterFullScreenExit();
        void ResetLogicalMaximizedState() const noexcept;
        void RestoreLogicalMaximizedState() const noexcept;
        void StopInteractiveWindowOperation() const noexcept;

        void AttachWebView();
        void AddCustomSchemeHandlers();

        void SetUserAgent(const PlatformString& userAgent);
        void ConfigureWebViewPreferences();
#endif
    public:
#ifdef _WIN32
        void ApplySizeLimits(MINMAXINFO& info) const noexcept;
        void RefitContent() const;
        void FocusWebView2() const;
        void CloseWebView();
        bool RefreshWindowIconsForDpi(UINT dpi);
#elif defined(__linux__)
        void HandleConfigureEvent(int x, int y, int width, int height);
        void HandleWindowStateEvent();

        void CompleteScheduledRestoreNormalGeometry();

        void UpdateWebViewInputShape() const noexcept;
#elif defined(__APPLE__)
        void HandleFullScreenExitCompleted() noexcept;
        void HandleMiniaturizeStarted() noexcept;
        void HandleMiniaturizeCompleted() noexcept;
        bool HasPendingStateAfterFullScreenExit() const noexcept;
        bool IsFullScreenTransitioning() const noexcept;
        void SetFullScreenTransitioning(bool value) noexcept;
#endif
        Photino(PhotinoInitParams* initParams);
        ~Photino();
#ifdef _WIN32
        static void Register(HINSTANCE hInstance);
        static void SetWebView2RuntimePath(const PlatformString& pathToWebView2);
        static const char* GetWebView2RuntimeVersion();
#elif defined(__linux__)
        static void Register();
        static const char* GetGtkVersion();
        static const char* GetGlibcVersion();
        static const char* GetWebKitGtkRuntimeVersion();
#elif defined(__APPLE__)
        static void Register();
        static const char* GetWebKitVersion();
#endif
        // Platform handles
#ifdef _WIN32
        HWND GetHwnd() const noexcept;
        WindowsState& Platform() noexcept { return *platform_; }
        const WindowsState& Platform() const noexcept { return *platform_; }
#elif defined(__linux__)
        void* GetGtkWidget() const noexcept;
        LinuxState& Platform() noexcept { return *platform_; }
        const LinuxState& Platform() const noexcept { return *platform_; }
#elif defined(__APPLE__)
        void* GetNSWindow() const noexcept;
        MacState& Platform() noexcept { return *platform_; }
        const MacState& Platform() const noexcept { return *platform_; }
#endif
        // Common platform state predicates
        bool IsFullScreen() const noexcept;
        bool IsMinimized() const noexcept;
        bool IsMaximized() const noexcept;

        bool UpdateWindowState() noexcept;

        bool Show() const;
        bool Activate() const;
        bool Center() const;
        bool Maximize();
        bool Minimize();
        bool Restore();
        void Close() const;

        // Window metadata
        const PlatformString& GetTitle() const noexcept { return options_.windowTitle; }
        void SetTitle(const PlatformString& title);

        const PlatformString& GetIconFile() const noexcept { return options_.iconFileName; }
        void SetIconFile(const PlatformString& filename);

        // Window geometry
        void GetPosition(int* x, int* y) const;
        void SetPosition(int x, int y);

        void GetSize(int* width, int* height) const;
        void SetSize(int width, int height);

        void SetMinSize(int width, int height);
        void SetMaxSize(int width, int height);

        bool CanBeginResize() const noexcept;
        bool CanBeginDrag() const noexcept;
        void BeginWindowDrag() const;
        void BeginWindowResize(PhotinoWindowEdge edge) const;

        unsigned int GetScreenDpi() const;
        bool GetAllMonitors(GetAllMonitorsCallback callback, void* state) const noexcept;
        bool GetWindowMonitor(Monitor& monitor) const noexcept;

        // Window state
        void GetWindowState(PhotinoWindowState* state) const;
        void SetWindowState(const PhotinoWindowState state);

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
        PhotinoDialog* GetDialog() const noexcept { return dialog_; }

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

        void GetStatusBarEnabled(bool* enabled) const;
        void SetStatusBarEnabled(bool enabled);

        void GetGrantBrowserPermissions(bool* grant) const;
        const PlatformString& GetUserAgent() const noexcept { return options_.userAgent; }
        void GetMediaAutoplayEnabled(bool* enabled) const;
        void GetFileSystemAccessEnabled(bool* enabled) const;
        void GetWebSecurityEnabled(bool* enabled) const;
        void GetJavascriptClipboardAccessEnabled(bool* enabled) const;
        void GetMediaStreamEnabled(bool* enabled) const;
        void GetSmoothScrollingEnabled(bool* enabled) const;
        void GetIgnoreCertificateErrorsEnabled(bool* enabled) const;

        // Callbacks
        void SetClosingCallback(ClosingCallback callback) noexcept { closingCallback_ = callback; }
        void SetClosedCallback(ClosedCallback callback) noexcept { closedCallback_ = callback; }
        void SetFocusInCallback(FocusInCallback callback) noexcept { focusInCallback_ = callback; }
        void SetFocusOutCallback(FocusOutCallback callback) noexcept { focusOutCallback_ = callback; }
        void SetMovedCallback(MovedCallback callback) noexcept { movedCallback_ = callback; }
        void SetResizedCallback(ResizedCallback callback) noexcept { resizedCallback_ = callback; }
        void SetMaximizedCallback(MaximizedCallback callback) noexcept { maximizedCallback_ = callback; }
        void SetRestoredCallback(RestoredCallback callback) noexcept { restoredCallback_ = callback; }
        void SetMinimizedCallback(MinimizedCallback callback) noexcept { minimizedCallback_ = callback; }
        void SetFullScreenChangedCallback(FullScreenChangedCallback callback) noexcept { fullScreenChangedCallback_ = callback; }
        void SetStateChangedCallback(StateChangedCallback callback) noexcept { stateChangedCallback_ = callback; }
        void SetContentLoadingCallback(ContentLoadingCallback callback) noexcept { contentLoadingCallback_ = callback; }
        void SetContentLoadedCallback(ContentLoadedCallback callback) noexcept { contentLoadedCallback_ = callback; }
        void SetNavigationStartingCallback(NavigationStartingCallback callback) noexcept { navigationStartingCallback_ = callback; }
        void SetNewWindowRequestedCallback(NewWindowRequestedCallback callback) noexcept { newWindowRequestedCallback_ = callback; }

        // Callback invokers
        bool InvokeClosing() const noexcept;
        void InvokeClose() const noexcept;
        void InvokeFocusIn() const noexcept;
        void InvokeFocusOut() const noexcept;
        void InvokeMove(int x, int y) const noexcept;
        void InvokeResize(int width, int height) const noexcept;
        void InvokeMaximized() const noexcept;
        void InvokeRestored() const noexcept;
        void InvokeMinimized() const noexcept;
        void InvokeFullScreenChanged(bool fullScreen) const noexcept;
        void InvokeStateChanged(PhotinoWindowState oldState, PhotinoWindowState newState) const noexcept;
        void InvokeWebMessageReceived(const PlatformString& message, const PlatformString& uri) const noexcept;
        void InvokeContentLoading(const PlatformString& uri) const noexcept;
        void InvokeContentLoaded(const PlatformString& uri) const noexcept;
        bool InvokeNavigationStarting(const PlatformString& uri) const noexcept;
        bool InvokeNewWindowRequested(const PlatformString& uri) const noexcept;
    };

} // namespace PhotinoX::Native