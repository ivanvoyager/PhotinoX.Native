#include "Photino.h"
#include "Photino.Dialog.h"
#include "Photino.Memory.h"
#include "Dependencies/json.hpp"

#include <algorithm>
#include <atomic>
#include <cassert>
#include <condition_variable>
#include <csignal>
#include <memory>
#include <mutex>
#include <JavaScriptCore/JavaScript.h>
#include <X11/Xlib.h>
#include <libnotify/notify.h>
#include <webkit2/webkit2.h>

using json = nlohmann::json;

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


struct GObjectDeleter
{
    void operator()(gpointer object) const noexcept
    {
        if (object)
            g_object_unref(object);
    }
};

template <typename T>
using GObjectPtr = std::unique_ptr<T, GObjectDeleter>;

struct WebKitUserScriptDeleter
{
    void operator()(WebKitUserScript* script) const noexcept
    {
        if (script)
            webkit_user_script_unref(script);
    }
};

using WebKitUserScriptPtr = std::unique_ptr<WebKitUserScript, WebKitUserScriptDeleter>;


struct InvokeWaitInfo
{
    InvokeCallback callback = nullptr;
    std::condition_variable completionNotifier;
    std::mutex mutex;
    bool isCompleted = false;
};

struct InvokeJSWaitInfo
{
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

Photino::Photino(PhotinoInitParams* initParams)
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

    _window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    if (!_window)
        std::abort();

    _dialog = new PhotinoDialog();

    _notifyInitialized = _notificationsEnabled && AcquireNotifications(_windowTitle);

    if (initParams->FullScreen)
    {
        SetFullScreen(true);
    }
    else
    {
        if (initParams->UseOsDefaultSize)
        {
            gtk_window_set_default_size(GTK_WINDOW(_window), -1, -1);
        }
        else
        {
            // Ensure that the default size does not exceed any set min/max dimension
            if (_sizeLimits.maxWidth > 0 && initParams->Width > _sizeLimits.maxWidth)
                initParams->Width = _sizeLimits.maxWidth;
            if (_sizeLimits.maxHeight > 0 && initParams->Height > _sizeLimits.maxHeight)
                initParams->Height = _sizeLimits.maxHeight;
            if (_sizeLimits.minWidth > 0 && initParams->Width < _sizeLimits.minWidth)
                initParams->Width = _sizeLimits.minWidth;
            if (_sizeLimits.minHeight > 0 && initParams->Height < _sizeLimits.minHeight)
                initParams->Height = _sizeLimits.minHeight;

            if (initParams->Width < 0)  initParams->Width = -1;
            if (initParams->Height < 0) initParams->Height = -1;
            gtk_window_set_default_size(GTK_WINDOW(_window), initParams->Width, initParams->Height);
        }

        SetMinSize(_sizeLimits.minWidth, _sizeLimits.minHeight);
        SetMaxSize(_sizeLimits.maxWidth, _sizeLimits.maxHeight);

        if (initParams->UseOsDefaultLocation)
            gtk_window_set_position(GTK_WINDOW(_window), GTK_WIN_POS_NONE);
        else if (initParams->CenterOnInitialize && !initParams->FullScreen)
            gtk_window_set_position(GTK_WINDOW(_window), GTK_WIN_POS_CENTER);
        else
            gtk_window_move(GTK_WINDOW(_window), initParams->Left, initParams->Top);
    }

    SetTitle(_windowTitle);

    if (initParams->Chromeless)
        gtk_window_set_decorated(GTK_WINDOW(_window), false);

    SetIconFile(ToPlatformString(initParams->WindowIconFile));

    if (initParams->Minimized)
        SetMinimized(true);

    if (initParams->Maximized)
        SetMaximized(true);

    if (!initParams->Resizable)
        SetResizable(false);

    if (initParams->Topmost)
        SetTopmost(true);

    // g_signal_connect(G_OBJECT(_window), "size-allocate",
    //	G_CALLBACK(on_size_allocate),
    //	this);

    g_signal_connect(G_OBJECT(_window), "configure-event",
                     G_CALLBACK(on_configure_event),
                     this);

    g_signal_connect(G_OBJECT(_window), "window-state-event",
                     G_CALLBACK(on_window_state_event),
                     this);

    g_signal_connect(G_OBJECT(_window), "delete-event",
                     G_CALLBACK(on_widget_deleted),
                     this);

    g_signal_connect(G_OBJECT(_window), "destroy",
                     G_CALLBACK(on_widget_destroyed),
                     this);

    if (initParams->Transparent)
        SetTransparentEnabled(true);//visual/app-paintable

    Show();

    if (!_webview)
        std::abort();

    if (initParams->Transparent)
        SetTransparentEnabled(true);//WebKit background alpha

    g_signal_connect(G_OBJECT(_window), "focus-in-event",
                     G_CALLBACK(on_focus_in_event),
                     this);

    g_signal_connect(G_OBJECT(_window), "focus-out-event",
                     G_CALLBACK(on_focus_out_event),
                     this);

    if (_zoom != 100.0)
        SetZoom(_zoom);

    // gchar* webkitVer = g_strconcat(g_strdup_printf("%d", webkit_get_major_version()), ".", g_strdup_printf("%d", webkit_get_minor_version()), ".", g_strdup_printf("%d", webkit_get_micro_version()), NULL);
    // Photino::ShowNotification("Web Kit Version", webkitVer);
}

Photino::~Photino()
{
    if (_notifyInitialized)
        ReleaseNotifications();

    delete _dialog;
}

void Photino::Center()
{
    assert(_window);
    if (!_window) return;

    gint windowWidth = 0, windowHeight = 0;
    gtk_window_get_size(GTK_WINDOW(_window), &windowWidth, &windowHeight);

    GdkRectangle screen = {0};

    GdkDisplay* display = gdk_display_get_default();
    if (display == NULL)
    {
        GtkWidget *dialog = gtk_message_dialog_new(
            nullptr, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "gdk_display_get_default() returned NULL");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        return;
    }

    GdkWindow* gdkWindow = gtk_widget_get_window(_window);
    GdkMonitor* monitor = gdkWindow
        ? gdk_display_get_monitor_at_window(display, gdkWindow)
        : gdk_display_get_primary_monitor(display);

    if (monitor == NULL)
    {
        monitor = gdk_display_get_monitor(display, 0); // Attempt to get the first monitor
        if (monitor == NULL)
        {
            GtkWidget *dialog = gtk_message_dialog_new(
                nullptr, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "gdk_display_get_monitor() returned NULL");
            gtk_dialog_run(GTK_DIALOG(dialog));
            gtk_widget_destroy(dialog);
            return;
        }
    }

    gdk_monitor_get_workarea(monitor, &screen);

    gtk_window_move(GTK_WINDOW(_window),
                    screen.x + (screen.width - windowWidth) / 2,
                    screen.y + (screen.height - windowHeight) / 2);
}

void Photino::ClearBrowserAutoFill() const
{
    // TODO
}



void Photino::GetTransparentEnabled(bool* enabled) const
{
    assert(enabled);
    if (!enabled) return;

    *enabled = _transparentEnabled;
}

void Photino::GetContextMenuEnabled(bool* enabled) const
{
    assert(enabled);
    if (!enabled) return;

    *enabled = _contextMenuEnabled;
}

void Photino::GetZoomEnabled(bool* enabled) const
{
    assert(enabled);
    if (!enabled) return;

    *enabled = _zoomEnabled;
}

void Photino::GetDevToolsEnabled(bool* enabled) const
{
    assert(enabled);
    if (!enabled) return;

    *enabled = _devToolsEnabled;

    if (!_webview) return;

    WebKitSettings* settings = webkit_web_view_get_settings(WEBKIT_WEB_VIEW(_webview));
    if (!settings)
        return;

    *enabled = webkit_settings_get_enable_developer_extras(settings) ? true : false;
}

void Photino::GetFullScreen(bool* fullScreen) const
{
    assert(fullScreen);
    if (!fullScreen) return;

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

    *enabled = _notifyInitialized;
}

void Photino::GetMaximized(bool* isMaximized) const
{
    assert(isMaximized);
    if (!isMaximized) return;

    *isMaximized = false;

    if (!_window) return;

    // gboolean maximized = gtk_window_is_maximized(GTK_WINDOW(_window));  //this method doesn't work
    //*isMaximized = maximized;
    GdkWindow* gdk_window = gtk_widget_get_window(GTK_WIDGET(_window));
    if (!gdk_window)
        return;

    GdkWindowState flags = gdk_window_get_state(gdk_window);
    *isMaximized = (flags & GDK_WINDOW_STATE_MAXIMIZED) != 0;
}

void Photino::GetMinimized(bool* isMinimized) const
{
    assert(isMinimized);
    if (!isMinimized) return;

    *isMinimized = false;

    if (!_window) return;

    GdkWindow* gdk_window = gtk_widget_get_window(GTK_WIDGET(_window));
    if (gdk_window == NULL)
        return;

    GdkWindowState flags = gdk_window_get_state(gdk_window);
    *isMinimized = (flags & GDK_WINDOW_STATE_ICONIFIED) != 0;
}

void Photino::GetPosition(int* x, int* y) const
{
    assert(x || y);
    if (!x && !y) return;

    if (x) *x = 0;
    if (y) *y = 0;

    if (!_window) return;

    gint windowX = 0;
    gint windowY = 0;
    gtk_window_get_position(GTK_WINDOW(_window), &windowX, &windowY);

    if (x) *x = windowX;
    if (y) *y = windowY;
}

void Photino::GetResizable(bool* resizable) const
{
    assert(resizable);
    if (!resizable) return;

    *resizable = false;

    if (!_window) return;

    *resizable = gtk_window_get_resizable(GTK_WINDOW(_window)) != FALSE;
}

unsigned int Photino::GetScreenDpi() const
{
    if (!_window) return 96;

    GdkScreen *screen = gtk_window_get_screen(GTK_WINDOW(_window));
    if (!screen) return 96;

    gdouble dpi = gdk_screen_get_resolution(screen);
    return dpi < 0 ? 96 : static_cast<unsigned int>(dpi);
}

void Photino::GetSize(int* width, int* height) const
{
    assert(width || height);
    if (!width && !height) return;

    if (width)  *width = 0;
    if (height) *height = 0;

    if (!_window) return;

    gint windowWidth = 0;
    gint windowHeight = 0;
    gtk_window_get_size(GTK_WINDOW(_window), &windowWidth, &windowHeight);

    if (width) *width = windowWidth;
    if (height) *height = windowHeight;

    // TODO: When calling set height, then set width...
    // calling set size works fine.
    // Uncomment this and it works properly. Commented, it only changes width.
    // GtkWidget* dialog = gtk_message_dialog_new(
    // 	nullptr
    // 	, GTK_DIALOG_DESTROY_WITH_PARENT
    // 	, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE
    // 	, "width: %i bytes, height %i"
    // 	, *width
    // 	, *height);
    // gtk_dialog_run(GTK_DIALOG(dialog));
    // gtk_widget_destroy(dialog);
}

/*AutoString Photino::GetTitle() const
{
    return (AutoString)gtk_window_get_title(GTK_WINDOW(_window));
}
*/

void Photino::GetTopmost(bool* topmost) const
{
    assert(topmost);
    if (!topmost) return;

    *topmost = false;

    if (!_window) return;

    // TODO: This flag is not set in GDK3. WebKit does not support GTK5 yet.
    GdkWindow* gdk_window = gtk_widget_get_window(GTK_WIDGET(_window));
    if (!gdk_window) return;

    GdkWindowState flags = gdk_window_get_state(gdk_window);
    *topmost = (flags & GDK_WINDOW_STATE_ABOVE) != 0;

    // char tmp1[FMT_BUF_SIZE];
    // char tmp2[FMT_BUF_SIZE];
    // char tmp3[FMT_BUF_SIZE];
    // GtkWidget* dialog = gtk_message_dialog_new(
    //	nullptr
    //	, GTK_DIALOG_DESTROY_WITH_PARENT
    //	, GTK_MESSAGE_ERROR
    //	, GTK_BUTTONS_CLOSE
    //	, "flags: %s \n above: %s \n and: %s \n topmost: %s"
    //	, binary_fmt(flags, tmp1)
    //	, binary_fmt(GDK_WINDOW_STATE_ABOVE, tmp2)
    //	, binary_fmt(flags & GDK_WINDOW_STATE_ABOVE, tmp3)
    //	, *topmost ? "T" : "F");
    // gtk_dialog_run(GTK_DIALOG(dialog));
    // gtk_widget_destroy(dialog);
}

void Photino::GetZoom(int* zoom) const
{
    assert(zoom);
    if (!zoom) return;

    *zoom = _zoom;

    if (!_webview) return;

    double rawValue = webkit_web_view_get_zoom_level(WEBKIT_WEB_VIEW(_webview));
    rawValue = (rawValue * 100.0) + 0.5;
    *zoom = static_cast<int>(rawValue);
}

void Photino::NavigateToString(const PlatformString& content) const
{
    assert(_webview);
    if (!_webview) return;

    webkit_web_view_load_html(WEBKIT_WEB_VIEW(_webview), content.c_str(), nullptr);
}

void Photino::NavigateToUrl(const PlatformString& url) const
{
    assert(_webview);
    if (!_webview || url.empty()) return;

    webkit_web_view_load_uri(WEBKIT_WEB_VIEW(_webview), url.c_str());
}

void Photino::Restore() const
{
    assert(_window);
    if (!_window) return;

    gtk_window_present(GTK_WINDOW(_window));
}

static void webview_eval_finished(GObject* object, GAsyncResult* result, gpointer userdata)
{
    auto waitInfo = static_cast<InvokeJSWaitInfo*>(userdata);
    if (!waitInfo) return;

    GError* error = nullptr;
    JSCValue* value = webkit_web_view_evaluate_javascript_finish(WEBKIT_WEB_VIEW(object), result, &error);

    if (error)
    {
        g_warning("Failed to dispatch message to WebView: %s", error->message);
        g_error_free(error);
    }

    if (value)
        g_object_unref(value);

    waitInfo->isCompleted = true;
}

void Photino::SendWebMessage(const PlatformString &message) const
{
    assert(_webview);
    if (!_webview)
        return;

    PlatformString js;
    js.append("__dispatchMessageCallback(");
    js.append(json(message).dump(-1, ' ', false, json::error_handler_t::replace));
    js.append(")");

    InvokeJSWaitInfo invokeJsWaitInfo{};

    webkit_web_view_evaluate_javascript(
        WEBKIT_WEB_VIEW(_webview),
        js.c_str(),
        -1,
        nullptr,
        nullptr,
        nullptr,
        webview_eval_finished,
        &invokeJsWaitInfo);

    while (!invokeJsWaitInfo.isCompleted)
        g_main_context_iteration(nullptr, TRUE);
}

void Photino::SetContextMenuEnabled(bool enabled)
{
    _contextMenuEnabled = enabled;
}

void Photino::SetZoomEnabled(bool enabled)
{
    _zoomEnabled = enabled;
    //! Not implemented (supported?) on Linux
}

void Photino::SetDevToolsEnabled(bool enabled)
{
    _devToolsEnabled = enabled;

    if (!_webview) return;

    WebKitSettings *settings = webkit_web_view_get_settings(WEBKIT_WEB_VIEW(_webview));
    if (!settings)  return;

    webkit_settings_set_enable_developer_extras(settings, enabled);
}

void Photino::SetFullScreen(bool fullScreen)
{
    _fullScreen = fullScreen;

    assert(_window);
    if (!_window)
        return;

    if (fullScreen)
        gtk_window_fullscreen(GTK_WINDOW(_window));
    else
        gtk_window_unfullscreen(GTK_WINDOW(_window));
}

void Photino::SetIconFile(const PlatformString& filename)
{
    assert(_window);
    if (!_window || filename.empty()) return;

    GError *error = nullptr;
    if (gtk_window_set_icon_from_file(GTK_WINDOW(_window), filename.c_str(), &error))
    {
        _iconFileName = filename;
        return;
    }

    if (error)
    {
        g_warning("Failed to set window icon: %s", error->message);
        g_error_free(error);
    }
}

void Photino::SetMinimized(bool minimized)
{
    assert(_window);
    if (!_window) return;

    if (minimized)
        gtk_window_iconify(GTK_WINDOW(_window));
    else
        gtk_window_deiconify(GTK_WINDOW(_window));
}

void Photino::SetMaximized(bool maximized)
{
    assert(_window);
    if (!_window) return;

    if (maximized)
        gtk_window_maximize(GTK_WINDOW(_window));
    else
        gtk_window_unmaximize(GTK_WINDOW(_window));
}

void Photino::SetPosition(int x, int y)
{
    assert(_window);
    if (!_window) return;

    gtk_window_move(GTK_WINDOW(_window), x, y);
}

void Photino::SetResizable(bool resizable)
{
    assert(_window);
    if (!_window) return;

    gtk_window_set_resizable(GTK_WINDOW(_window), resizable);
}

void Photino::SetMinSize(int width, int height)
{
    _sizeLimits.minWidth = (std::max)(0, width);
    _sizeLimits.minHeight = (std::max)(0, height);

    if (_sizeLimits.maxWidth > 0 && _sizeLimits.minWidth > _sizeLimits.maxWidth)
        _sizeLimits.maxWidth = _sizeLimits.minWidth;

    if (_sizeLimits.maxHeight > 0 && _sizeLimits.minHeight > _sizeLimits.maxHeight)
        _sizeLimits.maxHeight = _sizeLimits.minHeight;

    ApplyGeometryHints();
}

void Photino::SetMaxSize(int width, int height)
{

    _sizeLimits.maxWidth = (std::max)(0, width);
    _sizeLimits.maxHeight = (std::max)(0, height);

    if (_sizeLimits.maxWidth > 0 && _sizeLimits.maxWidth < _sizeLimits.minWidth)
        _sizeLimits.minWidth = _sizeLimits.maxWidth;

    if (_sizeLimits.maxHeight > 0 && _sizeLimits.maxHeight < _sizeLimits.minHeight)
        _sizeLimits.minHeight = _sizeLimits.maxHeight;

    ApplyGeometryHints();
}

void Photino::SetSize(int width, int height)
{
    assert(_window);
    if (!_window) return;

    if (width <= 0 || height <= 0) return;

    int newWidth = width;
    int newHeight = height;

    if (_sizeLimits.minWidth > 0 && newWidth < _sizeLimits.minWidth)    newWidth = _sizeLimits.minWidth;
    if (_sizeLimits.minHeight > 0 && newHeight < _sizeLimits.minHeight) newHeight = _sizeLimits.minHeight;
    if (_sizeLimits.maxWidth > 0 && newWidth > _sizeLimits.maxWidth)    newWidth = _sizeLimits.maxWidth;
    if (_sizeLimits.maxHeight > 0 && newHeight > _sizeLimits.maxHeight) newHeight = _sizeLimits.maxHeight;

    gtk_window_resize(GTK_WINDOW(_window), newWidth, newHeight);
}

void Photino::ApplyGeometryHints()
{
    if (!_window) return;

    _hints.min_width = _sizeLimits.minWidth;
    _hints.min_height = _sizeLimits.minHeight;
    _hints.max_width = _sizeLimits.maxWidth > 0 ? _sizeLimits.maxWidth : G_MAXINT;
    _hints.max_height = _sizeLimits.maxHeight > 0 ? _sizeLimits.maxHeight : G_MAXINT;

    gtk_window_set_geometry_hints(
        GTK_WINDOW(_window),
        nullptr,
        &_hints,
        static_cast<GdkWindowHints>(GDK_HINT_MIN_SIZE | GDK_HINT_MAX_SIZE));
}

void Photino::SetTitle(const PlatformString& title)
{
    assert(_window);
    if (!_window)  return;

    gtk_window_set_title(GTK_WINDOW(_window), title.c_str());
    _windowTitle = title;
}

void Photino::SetTopmost(bool topmost)
{
    assert(_window);
    if (!_window) return;

    gtk_window_set_keep_above(GTK_WINDOW(_window), topmost);
}

void Photino::SetZoom(int zoom)
{
    if (zoom < 25)
        zoom = 25;
    else if (zoom > 500)
        zoom = 500;
    _zoom = zoom;

    if (!_webview) return;

    double newZoom = static_cast<double>(zoom) / 100.0;
    webkit_web_view_set_zoom_level(WEBKIT_WEB_VIEW(_webview), newZoom);
}

void Photino::SetTransparentEnabled(bool enabled)
{
    _transparentEnabled = enabled;

    assert(_window);
    if (!_window) return;

    gtk_window_set_decorated(GTK_WINDOW(_window), !_chromeless && !enabled); // hide/show window chrome

    GdkScreen* screen = gtk_window_get_screen(GTK_WINDOW(_window));
    if (!screen) return;

    GdkVisual* rgbaVisual = gdk_screen_get_rgba_visual(screen);
    if (!rgbaVisual) return;

    gtk_widget_set_visual(GTK_WIDGET(_window), rgbaVisual);
    gtk_widget_set_app_paintable(GTK_WIDGET(_window), true);

    if (!_webview) return;

    GdkRGBA color;
    webkit_web_view_get_background_color(WEBKIT_WEB_VIEW(_webview), &color);

    color.alpha = enabled ? 0 : 1;

    webkit_web_view_set_background_color(WEBKIT_WEB_VIEW(_webview), &color);
}

void Photino::ShowNotification(const PlatformString& title, const PlatformString& message) const
{
    if (!_notifyInitialized)
        return;

    NotifyNotification* notification = notify_notification_new(title.c_str(), message.c_str(), nullptr);
    if (!notification)
        return;

    if (_window)
    {
        GdkPixbuf* icon = gtk_window_get_icon(GTK_WINDOW(_window));
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
void Photino::GetAllMonitors(GetAllMonitorsCallback callback) const noexcept
{
    assert(callback);
    if (!callback) return;

    assert(_window);
    if (!_window) return;

    GdkScreen* screen = gtk_window_get_screen(GTK_WINDOW(_window));
    if (!screen) return;

    GdkDisplay* display = gdk_screen_get_display(screen);
    if (!display) return;

    int n = gdk_display_get_n_monitors(display);
    for (int i = 0; i < n; i++)
    {
        GdkMonitor* monitor = gdk_display_get_monitor(display, i);
        if (!monitor) continue;

        GdkRectangle monitorArea{};
        GdkRectangle workArea{};
        gdk_monitor_get_geometry(monitor, &monitorArea);
        gdk_monitor_get_workarea(monitor, &workArea);

        Monitor props{};
        props.monitor.x = monitorArea.x;
        props.monitor.y = monitorArea.y;
        props.monitor.width = monitorArea.width;
        props.monitor.height = monitorArea.height;
        props.work.x = workArea.x;
        props.work.y = workArea.y;
        props.work.width = workArea.width;
        props.work.height = workArea.height;
        props.scale = gdk_monitor_get_scale_factor(monitor);

        if (!callback(&props))
            break;
    }
}

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
    if (!_webview)
    {
        struct sigaction old_action{};
        bool hasOldSigchldAction = sigaction(SIGCHLD, nullptr, &old_action) == 0;

        GObjectPtr<WebKitUserContentManager> contentManager(webkit_user_content_manager_new());
        if (!contentManager)
            std::abort();

        _webview = webkit_web_view_new_with_user_content_manager(contentManager.get());
        if (!_webview)
            std::abort();

        SetWebKitSettings();

        // this may or may not work
        // g_object_set(G_OBJECT(settings), "enable-auto-fill-form", TRUE, NULL);

        gtk_container_add(GTK_CONTAINER(_window), _webview);

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
        g_signal_connect(G_OBJECT(_webview), "context-menu",
                         G_CALLBACK(on_webview_context_menu),
                         this);

        g_signal_connect(G_OBJECT(_webview), "permission-request",
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

    gtk_widget_show_all(_window);
}

void Photino::SetWebKitSettings()
{
    assert(_webview);
    if (!_webview) return;

    // Rely on webkit_settings_new_with_settings to set the default settings
    // instead of using the webkit2gtk API to set the properties.
    // https://webkitgtk.org/reference/webkitgtk/stable/ctor.Settings.new_with_settings.html
    GObjectPtr<WebKitSettings> settings(webkit_settings_new_with_settings(
        // Set Photino-specific default settings
        "allow_modal_dialogs", TRUE,                                            // default: FALSE
        "allow_top_navigation_to_data_urls", TRUE,                              // default: FALSE
        "allow_universal_access_from_file_urls", TRUE,                          // default: FALSE
        "enable_back_forward_navigation_gestures", TRUE,                        // default: FALSE
        "enable_media_capabilities", TRUE,                                      // default: FALSE
        "enable_mock_capture_devices", TRUE,                                    // default: FALSE
        "enable_page_cache", TRUE,                                              // default: FALSE
        "enable_webrtc", TRUE,                                                  // default: FALSE
        "javascript_can_open_windows_automatically", TRUE,                      // default: FALSE

        // Set user-defined settings
        "allow_file_access_from_file_urls", _fileSystemAccessEnabled,           // default: FALSE
        "disable_web_security", !_webSecurityEnabled,                           // default: FALSE
        "enable_developer_extras", _devToolsEnabled,                            // default: FALSE
        "enable_media_stream", _mediaStreamEnabled,                             // default: FALSE
        "enable_smooth_scrolling", _smoothScrollingEnabled,                     // default: TRUE
        "javascript_can_access_clipboard", _javascriptClipboardAccessEnabled,   // default: FALSE
        "media_playback_requires_user_gesture", !_mediaAutoplayEnabled,         // default: FALSE
        "user_agent", !_userAgent.empty() ? _userAgent.c_str() : nullptr,       // default: None

        // Other available settings for reference
        // "default_charset", "iso-8859-1",										// default: iso-8859-1
        // "cursive_font_family", "serif",										// default: serif
        // "default_font_family", "sans-serif",									// default: sans-serif
        // "fantasy_font_family", "serif",										// default: serif
        // "monospace_font_family", "monospace",								// default: monospace
        // "pictograph_font_family", "serif",									// default: serif
        // "sans_serif_font_family", "sans-serif",								// default: sans-serif
        // "minimum_font_size", 0,												// default: 0
        // "default_font_size", 16,												// default: 16
        // "default_monospace_font_size", 13,									// default: 13
        // "auto_load_images", TRUE,											// default: TRUE
        // "enable_fullscreen", TRUE,											// default: TRUE
        // "enable_html5_database", TRUE,										// default: TRUE
        // "enable_html5_local_storage", TRUE,									// default: TRUE
        // "enable_hyperlink_auditing", TRUE,									// default: TRUE
        // "enable_javascript", TRUE,											// default: TRUE
        // "enable_javascript_markup", TRUE,									// default: TRUE
        // "enable_media", TRUE,												// default: TRUE
        // "enable_mediasource", TRUE,											// default: TRUE
        // "enable_offline_web_application_cache", TRUE,						// default: TRUE
        // "enable_resizable_text_areas", TRUE,									// default: TRUE
        // "enable_site_specific_quirks", TRUE,									// default: TRUE
        // "enable_tabs_to_links", TRUE,										// default: TRUE
        // "enable_webaudio", TRUE,												// default: TRUE
        // "enable_webgl", TRUE,												// default: TRUE
        // "enable_xss_auditor", TRUE,											// default: TRUE
        // "media_playback_allows_inline", TRUE,								// default: TRUE
        // "print_backgrounds", TRUE,											// default: TRUE
        // "draw_compositing_indicators", FALSE,								// default: FALSE
        // "enable_accelerated_2d_canvas", FALSE,								// default: FALSE
        // "enable_caret_browsing", FALSE,										// default: FALSE
        // "enable_dns_prefetching", FALSE,										// default: FALSE
        // "enable_encrypted_media", FALSE,										// default: FALSE
        // "enable_frame_flattening", FALSE,									// default: FALSE
        // "enable_java", FALSE,												// default: FALSE
        // "enable_plugins", FALSE,												// default: FALSE
        // "enable_private_browsing", FALSE,									// default: FALSE
        // "enable_spatial_navigation", FALSE,									// default: FALSE
        // "enable_write_console_messages_to_stdout", FALSE,					// default: FALSE
        // "load_icons_ignoring_image_load_setting", FALSE,						// default: FALSE
        // "zoom_text_only", FALSE, 											// default: FALSE
        // "media_content_types_requiring_hardware_support", None,				// default: None
        // "hardware_acceleration_policy", WEBKIT_HARDWARE_ACCELERATION_POLICY_ALWAYS,	// default: WEBKIT_HARDWARE_ACCELERATION_POLICY_ALWAYS
        nullptr)); // NULL terminates the list
    if (!settings)
        std::abort();

    if (!_browserControlInitParameters.empty())
        SetWebKitCustomSettings(settings.get()); // if any custom init parameters were passed, set them now.

    WebKitWebsiteDataManager* manager = webkit_web_view_get_website_data_manager(WEBKIT_WEB_VIEW(_webview));
    if (manager)
    {
        webkit_website_data_manager_set_tls_errors_policy(manager,
            _ignoreCertificateErrorsEnabled ? WEBKIT_TLS_ERRORS_POLICY_IGNORE : WEBKIT_TLS_ERRORS_POLICY_FAIL);
    }

    webkit_web_view_set_settings(WEBKIT_WEB_VIEW(_webview), settings.get()); // apply the settings to the webview
}

void Photino::SetWebKitCustomSettings(WebKitSettings* settings)
{
    assert(settings);
    if (!settings) return;

    // parse the JSON out of _browserControlInitParameters
    json data = json::parse(_browserControlInitParameters, nullptr, false);
    if (data.is_discarded() || !data.is_object())
    {
        GtkWidget* dialog = gtk_message_dialog_new(
            nullptr, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "Invalid WebKit custom settings JSON.");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        std::abort();
    }

    for (auto it = data.begin(); it != data.end(); ++it)
    {
        // Use g_object_set_property to set the property on the settings object
        // instead of relying on the webkit2gtk API to set the properties.
        // https://docs.gtk.org/gobject/method.Object.set_property.html
        std::string propertyName = it.key();
        const json& value = it.value();

        GValue propertyValue = G_VALUE_INIT;

        if (value.is_string())
        {
            g_value_init(&propertyValue, G_TYPE_STRING);
            g_value_set_string(&propertyValue, value.get<std::string>().c_str());
        }
        else if (value.is_boolean())
        {
            g_value_init(&propertyValue, G_TYPE_BOOLEAN);
            g_value_set_boolean(&propertyValue, value.get<bool>());
        }
        else if (value.is_number_integer())
        {
            g_value_init(&propertyValue, G_TYPE_INT);
            g_value_set_int(&propertyValue, value.get<int>());
        }
        else if (value.is_number_float())
        {
            g_value_init(&propertyValue, G_TYPE_DOUBLE);
            g_value_set_double(&propertyValue, value.get<double>());
        }
        else
        {
            // Throw an error
            GtkWidget *dialog = gtk_message_dialog_new(
                nullptr, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "Invalid value type for key: %s", propertyName.c_str());
            gtk_dialog_run(GTK_DIALOG(dialog));
            gtk_widget_destroy(dialog);
            std::abort();
        }

        g_object_set_property(G_OBJECT(settings), propertyName.c_str(), &propertyValue);
        g_value_unset(&propertyValue);
    }
}

void Photino::HandleConfigureEvent(int x, int y, int width, int height)
{
    if (_lastGeometry.left != x || _lastGeometry.top != y)
    {
        InvokeMove(x, y);
        _lastGeometry.left = x;
        _lastGeometry.top = y;
    }

    if (_lastGeometry.width != width || _lastGeometry.height != height)
    {
        InvokeResize(width, height);
        _lastGeometry.width = width;
        _lastGeometry.height = height;
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

static void FreeNativeMemory(gpointer data)
{
    FreeMemory(data);
}

static void FinishCustomSchemeRequestWithError(WebKitURISchemeRequest* request, const char* message)
{
    GError* error = g_error_new_literal(G_IO_ERROR, G_IO_ERROR_FAILED, message);
    webkit_uri_scheme_request_finish_error(request, error);
    g_error_free(error);
}

void HandleCustomSchemeRequest(WebKitURISchemeRequest* request, gpointer userData)
{
    assert(request);
    if (!request) return;

    auto callback = reinterpret_cast<WebResourceRequestedCallback>(userData);
    if (!callback)
    {
        FinishCustomSchemeRequestWithError(request, "Custom scheme callback is not registered.");
        return;
    }

    const gchar* uri = webkit_uri_scheme_request_get_uri(request);
    if (!uri || *uri == '\0')
    {
        FinishCustomSchemeRequestWithError(request, "Custom scheme request URI is empty.");
        return;
    }

    int numBytes = 0;
    Utf8String contentType = nullptr;
    void* responseData = callback(uri, &numBytes, &contentType);

    if (!responseData || numBytes <= 0)
    {
        FreeMemory(responseData);
        FreeString(const_cast<char*>(contentType));

        FinishCustomSchemeRequestWithError(request, "Custom scheme response is empty.");
        return;
    }

    GObjectPtr<GInputStream> stream(g_memory_input_stream_new_from_data(responseData, numBytes, FreeNativeMemory));
    if (!stream)
    {
        FreeMemory(responseData);
        FreeString(const_cast<char*>(contentType));

        FinishCustomSchemeRequestWithError(request, "Failed to create custom scheme response stream.");
        return;
    }

    webkit_uri_scheme_request_finish(
        request,
        stream.get(),
        numBytes,
        contentType);

    FreeString(const_cast<char*>(contentType));
}

void Photino::AddCustomSchemeHandlers()
{
    assert(_webview);
    if (!_webview || !_customSchemeCallback) return;

    WebKitWebContext* context = webkit_web_view_get_context(WEBKIT_WEB_VIEW(_webview));
    if (!context) return;

    for (const auto& scheme : _customSchemeNames)
    {
        webkit_web_context_register_uri_scheme(
            context,
            scheme.c_str(),
            HandleCustomSchemeRequest,
            reinterpret_cast<void*>(_customSchemeCallback),
            nullptr);
    }
}

bool Photino::RegisterCustomSchemeName(const PlatformString& scheme)
{
    if (!_webview) return true;

    if (!_customSchemeCallback) return false;

    WebKitWebContext* context = webkit_web_view_get_context(WEBKIT_WEB_VIEW(_webview));
    if (!context) return false;

    if (scheme.empty()) return false;

    webkit_web_context_register_uri_scheme(
        context,
        scheme.c_str(),
        HandleCustomSchemeRequest,
        reinterpret_cast<void*>(_customSchemeCallback),
        nullptr);

    return true;
}