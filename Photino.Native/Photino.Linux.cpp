#include "Photino.h"
#include "Photino.Linux.Internal.h"
#include "Photino.Linux.State.h"
#include "Photino.Dialog.h"
#include "Photino.Strings.h"

#include <algorithm>
#include <atomic>
#include <cassert>
#include <condition_variable>
#include <csignal>
#include <memory>
#include <mutex>

#include <JavaScriptCore/JavaScript.h>
#include <X11/Xlib.h>
#include <gtk/gtk.h>
#include <libnotify/notify.h>
#include <webkit2/webkit2.h>

using namespace PhotinoX::Native;

namespace
{
    std::atomic_bool g_messageLoopRunning{ false };
    std::atomic_bool g_isShuttingDown{ false };
    Photino* g_messageLoopOwner = nullptr;

    std::mutex g_notifyMutex;
    int g_notifyRefCount = 0;
    bool g_notifyInitialized = false;

    bool AcquireNotifications(const PlatformString& appName)
    {
        std::lock_guard lock(g_notifyMutex);

        if (g_notifyInitialized)
        {
            g_notifyRefCount++;
            return true;
        }

        const char* name = appName.empty() ? "PhotinoX" : appName.c_str();

        if (!notify_init(name))
            return false;

        g_notifyInitialized = true;
        g_notifyRefCount = 1;
        return true;
    }

    void ReleaseNotifications()
    {
        std::lock_guard lock(g_notifyMutex);

        if (!g_notifyInitialized || g_notifyRefCount <= 0)
            return;

        g_notifyRefCount--;

        if (g_notifyRefCount == 0)
        {
            notify_uninit();
            g_notifyInitialized = false;
        }
    }
}

/* --- PRINTF_BINARY_FORMAT macro's --- */
// #define FMT_BUF_SIZE (CHAR_BIT*sizeof(uintmax_t)+1)
//
// char *binary_fmt(uintmax_t x, char buf[FMT_BUF_SIZE])
//{
//     char *s = buf + FMT_BUF_SIZE;
//     *--s = 0;
//     if (!x) *--s = '0';
//     for (; x; x /= 2) *--s = '0' + x%2;
//     return s;
// }
/* --- end macro --- */

struct InvokeWaitInfo
{
    InvokeCallback callback = nullptr;
    std::condition_variable completionNotifier;
    std::mutex mutex;
    bool isCompleted = false;
};


// window size or position changed
gboolean on_configure_event(GtkWidget* widget, GdkEvent* event, gpointer self);
gboolean on_window_state_event(GtkWidget* widget, GdkEventWindowState* event, gpointer self);
gboolean on_widget_deleted(GtkWidget* widget, GdkEvent* event, gpointer self);
void on_widget_destroyed(GtkWidget* widget, gpointer self);
gboolean on_focus_in_event(GtkWidget* widget, GdkEvent* event, gpointer self);
gboolean on_focus_out_event(GtkWidget* widget, GdkEvent* event, gpointer self);
gboolean on_webview_context_menu(WebKitWebView* web_view,
                                 GtkWidget* default_menu,
                                 WebKitHitTestResult* hit_test_result,
                                 gboolean triggered_with_keyboard,
                                 gpointer user_data);
gboolean on_permission_request(WebKitWebView* web_view, WebKitPermissionRequest* request, gpointer user_data);

void Photino::Register()
{
    static std::once_flag flag;
    std::call_once(flag, []() {
        XInitThreads();
        gtk_init(0, nullptr);
    });
}

Photino::Photino(PhotinoInitParams* initParams) : platform_(std::make_unique<LinuxState>())
{
    assert(initParams);
    if (!initParams)
        std::abort();

    if (initParams->Size != sizeof(PhotinoInitParams))
    {
        GtkWidget *dialog = gtk_message_dialog_new(
            nullptr, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
            "Initial parameters passed are %i bytes, but expected %zu bytes.",
            initParams->Size, sizeof(PhotinoInitParams));
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
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

    //these handlers are ALWAYS hooked up
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

    platform_->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    if (!platform_->window)
        std::abort();

    _dialog = new PhotinoDialog();

    platform_->notifyInitialized = _notificationsEnabled && AcquireNotifications(_windowTitle);

    if (initParams->FullScreen)
    {
        SetFullScreen(true);
    }
    else
    {
        if (initParams->UseOsDefaultSize)
        {
            gtk_window_set_default_size(GTK_WINDOW(platform_->window), -1, -1);
        }
        else
        {
            // Ensure that the default size does not exceed any set min/max dimension
            if (platform_->sizeLimits.maxWidth > 0 && initParams->Width > platform_->sizeLimits.maxWidth)
                initParams->Width = platform_->sizeLimits.maxWidth;
            if (platform_->sizeLimits.maxHeight > 0 && initParams->Height > platform_->sizeLimits.maxHeight)
                initParams->Height = platform_->sizeLimits.maxHeight;
            if (platform_->sizeLimits.minWidth > 0 && initParams->Width < platform_->sizeLimits.minWidth)
                initParams->Width = platform_->sizeLimits.minWidth;
            if (platform_->sizeLimits.minHeight > 0 && initParams->Height < platform_->sizeLimits.minHeight)
                initParams->Height = platform_->sizeLimits.minHeight;

            if (initParams->Width < 0)  initParams->Width = -1;
            if (initParams->Height < 0) initParams->Height = -1;
            gtk_window_set_default_size(GTK_WINDOW(platform_->window), initParams->Width, initParams->Height);
        }

        SetMinSize(platform_->sizeLimits.minWidth, platform_->sizeLimits.minHeight);
        SetMaxSize(platform_->sizeLimits.maxWidth, platform_->sizeLimits.maxHeight);

        if (initParams->UseOsDefaultLocation)
            gtk_window_set_position(GTK_WINDOW(platform_->window), GTK_WIN_POS_NONE);
        else if (initParams->CenterOnInitialize && !initParams->FullScreen)
            gtk_window_set_position(GTK_WINDOW(platform_->window), GTK_WIN_POS_CENTER);
        else
            gtk_window_move(GTK_WINDOW(platform_->window), initParams->Left, initParams->Top);
    }

    SetTitle(_windowTitle);

    if (initParams->Chromeless)
        gtk_window_set_decorated(GTK_WINDOW(platform_->window), false);

    SetIconFile(ToPlatformString(initParams->WindowIconFile));

    if (initParams->Minimized)
        SetMinimized(true);

    if (initParams->Maximized)
        SetMaximized(true);

    if (!initParams->Resizable)
        SetResizable(false);

    if (initParams->Topmost)
        SetTopmost(true);

    // g_signal_connect(G_OBJECT(platform_->window), "size-allocate",
    //	G_CALLBACK(on_size_allocate),
    //	this);

    g_signal_connect(G_OBJECT(platform_->window), "configure-event",
                     G_CALLBACK(on_configure_event),
                     this);

    g_signal_connect(G_OBJECT(platform_->window), "window-state-event",
                     G_CALLBACK(on_window_state_event),
                     this);

    g_signal_connect(G_OBJECT(platform_->window), "delete-event",
                     G_CALLBACK(on_widget_deleted),
                     this);

    g_signal_connect(G_OBJECT(platform_->window), "destroy",
                     G_CALLBACK(on_widget_destroyed),
                     this);

    if (initParams->Transparent)
        SetTransparentEnabled(true);//visual/app-paintable

    Show();

    if (!platform_->webview)
        std::abort();

    if (initParams->Transparent)
        SetTransparentEnabled(true);//WebKit background alpha

    g_signal_connect(G_OBJECT(platform_->window), "focus-in-event",
                     G_CALLBACK(on_focus_in_event),
                     this);

    g_signal_connect(G_OBJECT(platform_->window), "focus-out-event",
                     G_CALLBACK(on_focus_out_event),
                     this);

    if (_zoom != 100.0)
        SetZoom(_zoom);

    // gchar* webkitVer = g_strconcat(g_strdup_printf("%d", webkit_get_major_version()), ".", g_strdup_printf("%d", webkit_get_minor_version()), ".", g_strdup_printf("%d", webkit_get_micro_version()), NULL);
    // Photino::ShowNotification("Web Kit Version", webkitVer);
}

Photino::~Photino()
{
    if (platform_->notifyInitialized)
    {
        ReleaseNotifications();
        platform_->notifyInitialized = false;
    }

    delete _dialog;
}

void Photino::GetNotificationsEnabled(bool* enabled) const
{
    assert(enabled);
    if (!enabled) return;

    *enabled = platform_->notifyInitialized;
}

void Photino::ShowNotification(const PlatformString& title, const PlatformString& message) const
{
    if (!platform_->notifyInitialized) return;

    NotifyNotification* notification = notify_notification_new(title.c_str(), message.c_str(), nullptr);
    if (!notification)
        return;

    if (platform_->window)
    {
        GdkPixbuf* icon = gtk_window_get_icon(GTK_WINDOW(platform_->window));
        if (icon)
            notify_notification_set_icon_from_pixbuf(notification, icon);
    }

    GError* error = nullptr;
    notify_notification_show(notification, &error);

    if (error)
        g_error_free(error);

    g_object_unref(G_OBJECT(notification));
}

void Photino::WaitForExit() const
{
    bool expected = false;
    if (!g_messageLoopRunning.compare_exchange_strong(expected, true, std::memory_order_acq_rel)) return;

    g_isShuttingDown.store(false, std::memory_order_release);
    g_messageLoopOwner = const_cast<Photino*>(this);

    gtk_main();

    g_messageLoopOwner = nullptr;
    g_isShuttingDown.store(true, std::memory_order_release);
    g_messageLoopRunning.store(false, std::memory_order_release);
}

// Callbacks

static gboolean invokeCallback(gpointer data)
{
    auto waitInfo = static_cast<InvokeWaitInfo*>(data);
    if (!waitInfo) return G_SOURCE_REMOVE;

    if (waitInfo->callback)
        waitInfo->callback();

    {
        std::lock_guard<std::mutex> guard(waitInfo->mutex);
        waitInfo->isCompleted = true;
    }

    waitInfo->completionNotifier.notify_one();
    return G_SOURCE_REMOVE;
}

void Photino::Invoke(InvokeCallback callback) const
{
    assert(callback);
    if (!callback) return;

    if (g_isShuttingDown.load(std::memory_order_acquire)) return;

    auto context = g_main_context_default();
    if (!g_messageLoopRunning.load(std::memory_order_acquire) && !g_main_context_is_owner(context)) return;

    InvokeWaitInfo waitInfo{};
    waitInfo.callback = callback;

    g_main_context_invoke_full(
        context,
        G_PRIORITY_DEFAULT,
        invokeCallback,
        &waitInfo,
        nullptr);

    // Block until the callback is actually executed and completed
    std::unique_lock<std::mutex> lock(waitInfo.mutex);
    waitInfo.completionNotifier.wait(lock, [&waitInfo] {
        return waitInfo.isCompleted;
    });
}

// Private methods
void HandleWebMessage(WebKitUserContentManager* contentManager, WebKitJavascriptResult* jsResult, gpointer arg)
{
    if (!jsResult) return;

    JSCValue* jsValue = webkit_javascript_result_get_js_value(jsResult);
    if (!jsValue || !jsc_value_is_string(jsValue)) return;

    gchar* strValue = jsc_value_to_string(jsValue);
    if (!strValue) return;

    auto callback = reinterpret_cast<WebMessageReceivedCallback>(arg);
    if (callback)
        callback(strValue);

    g_free(strValue);
}

void Photino::Show()
{
    if (!platform_->webview)
    {
        struct sigaction old_action{};
        bool hasOldSigchldAction = sigaction(SIGCHLD, nullptr, &old_action) == 0;

        GObjectPtr<WebKitUserContentManager> contentManager(webkit_user_content_manager_new());
        if (!contentManager)
            std::abort();

        platform_->webview = webkit_web_view_new_with_user_content_manager(contentManager.get());
        if (!platform_->webview)
            std::abort();

        SetWebKitSettings();

        // this may or may not work
        // g_object_set(G_OBJECT(settings), "enable-auto-fill-form", TRUE, NULL);

        gtk_container_add(GTK_CONTAINER(platform_->window), platform_->webview);

        WebKitUserScriptPtr script(webkit_user_script_new(
            "window.__receiveMessageCallbacks = [];"
            "window.__dispatchMessageCallback = function(message) {"
            "	window.__receiveMessageCallbacks.forEach(function(callback) { callback(message); });"
            "};"
            "window.external = {"
            "	sendMessage: function(message) {"
            "		window.webkit.messageHandlers.Photinointerop.postMessage(message);"
            "	},"
            "	receiveMessage: function(callback) {"
            "		window.__receiveMessageCallbacks.push(callback);"
            "	}"
            "};",
            WEBKIT_USER_CONTENT_INJECT_ALL_FRAMES,
            WEBKIT_USER_SCRIPT_INJECT_AT_DOCUMENT_START,
            nullptr,
            nullptr));

        if (!script)
            std::abort();

        webkit_user_content_manager_add_script(contentManager.get(), script.get());

        g_signal_connect(contentManager.get(), "script-message-received::Photinointerop",
                         G_CALLBACK(HandleWebMessage), reinterpret_cast<void*>(_webMessageReceivedCallback));

        if (!webkit_user_content_manager_register_script_message_handler(contentManager.get(), "Photinointerop"))
            std::abort();

        // These must be called after the webview control is initialized.
        g_signal_connect(G_OBJECT(platform_->webview), "context-menu",
                         G_CALLBACK(on_webview_context_menu),
                         this);

        g_signal_connect(G_OBJECT(platform_->webview), "permission-request",
                         G_CALLBACK(on_permission_request),
                         this);

        AddCustomSchemeHandlers();

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
            GtkWidget *dialog = gtk_message_dialog_new(
                nullptr, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "Neither StartUrl not StartString was specified");
            gtk_dialog_run(GTK_DIALOG(dialog));
            gtk_widget_destroy(dialog);
            std::abort();
        }

        if (hasOldSigchldAction)
            sigaction(SIGCHLD, &old_action, nullptr);
    }

    gtk_widget_show_all(platform_->window);
}

void Photino::HandleConfigureEvent(int x, int y, int width, int height)
{
    if (platform_->lastGeometry.left != x || platform_->lastGeometry.top != y)
    {
        InvokeMove(x, y);
        platform_->lastGeometry.left = x;
        platform_->lastGeometry.top = y;
    }

    if (platform_->lastGeometry.width != width || platform_->lastGeometry.height != height)
    {
        InvokeResize(width, height);
        platform_->lastGeometry.width = width;
        platform_->lastGeometry.height = height;
    }
}

gboolean on_configure_event(GtkWidget* widget, GdkEvent* event, gpointer self)
{
    if (!event) return FALSE;

    if (event->type == GDK_CONFIGURE)
    {
        auto instance = static_cast<Photino*>(self);
        if (!instance) return FALSE;

        instance->HandleConfigureEvent(
            event->configure.x,
            event->configure.y,
            event->configure.width,
            event->configure.height);
    }

    return FALSE;
}

gboolean on_window_state_event(GtkWidget* widget, GdkEventWindowState* event, gpointer self)
{
    auto instance = static_cast<Photino*>(self);
    if (!instance || !event) return FALSE;

    if ((event->changed_mask & GDK_WINDOW_STATE_MAXIMIZED) &&
        (event->new_window_state & GDK_WINDOW_STATE_MAXIMIZED))
    {
        instance->InvokeMaximized();
        return FALSE;
    }

    if ((event->changed_mask & GDK_WINDOW_STATE_ICONIFIED) &&
        (event->new_window_state & GDK_WINDOW_STATE_ICONIFIED))
    {
        instance->InvokeMinimized();
        return FALSE;
    }

    if ((event->changed_mask & (GDK_WINDOW_STATE_MAXIMIZED | GDK_WINDOW_STATE_ICONIFIED)) &&
        !(event->new_window_state & GDK_WINDOW_STATE_MAXIMIZED) &&
        !(event->new_window_state & GDK_WINDOW_STATE_ICONIFIED))
    {
        instance->InvokeRestored();
    }

    return FALSE;
}

gboolean on_widget_deleted(GtkWidget* widget, GdkEvent* event, gpointer self)
{
    if (g_isShuttingDown.load(std::memory_order_acquire)) return FALSE;

    auto instance = static_cast<Photino*>(self);
    if (!instance) return FALSE;

    bool doNotClose = instance->InvokeClosing();
    return doNotClose ? TRUE : FALSE;
}

void on_widget_destroyed(GtkWidget* widget, gpointer self)
{
    auto instance = static_cast<Photino*>(self);
    if (!instance) return;

    instance->InvokeClose();

    if (instance == g_messageLoopOwner)
    {
        g_isShuttingDown.store(true, std::memory_order_release);
        g_messageLoopOwner = nullptr;
        gtk_main_quit();
    }

    delete instance;
}

gboolean on_focus_in_event(GtkWidget* widget, GdkEvent* event, gpointer self)
{
    auto instance = static_cast<Photino*>(self);
    if (!instance) return FALSE;

    instance->InvokeFocusIn();
    return FALSE;
}

gboolean on_focus_out_event(GtkWidget* widget, GdkEvent* event, gpointer self)
{
    auto instance = static_cast<Photino*>(self);
    if (!instance) return FALSE;

    instance->InvokeFocusOut();
    return FALSE;
}

gboolean on_webview_context_menu(WebKitWebView* web_view, GtkWidget* default_menu, WebKitHitTestResult* hit_test_result,
                                 gboolean triggered_with_keyboard, gpointer self)
{
    auto instance = static_cast<Photino*>(self);
    if (!instance) return FALSE;

    bool contextMenuEnabled = false;
    instance->GetContextMenuEnabled(&contextMenuEnabled);
    return !contextMenuEnabled ? TRUE : FALSE;
}

gboolean on_permission_request(WebKitWebView* web_view, WebKitPermissionRequest* request, gpointer self)
{
    auto instance = static_cast<Photino*>(self);
    if (!instance || !request) return FALSE;

    bool grantBrowserPermissions = false;
    instance->GetGrantBrowserPermissions(&grantBrowserPermissions);

    if (!grantBrowserPermissions)
        return FALSE;

    // GtkWidget *dialog = gtk_message_dialog_new(
    //	nullptr, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "Permission Requested - Allowing!");
    // gtk_dialog_run(GTK_DIALOG(dialog));
    //  gtk_widget_destroy(dialog);

    webkit_permission_request_allow(request);
    return TRUE;
}
