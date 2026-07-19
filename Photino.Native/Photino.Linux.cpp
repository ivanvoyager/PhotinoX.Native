#include "Photino.h"
#include "Photino.Linux.State.h"
#include "Photino.Dialog.h"
#include "Photino.Strings.h"
#include "Photino.Application.h"

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <memory>
#include <mutex>

#include <X11/Xlib.h>
#include <gtk/gtk.h>
#include <libnotify/notify.h>

using namespace PhotinoX::Native;

namespace
{
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

        if (event->changed_mask & GDK_WINDOW_STATE_FULLSCREEN)
        {
            instance->HandleFullScreenStateChanged(
                (event->new_window_state & GDK_WINDOW_STATE_FULLSCREEN) != 0);
        }

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

        if ((event->changed_mask & (GDK_WINDOW_STATE_MAXIMIZED | GDK_WINDOW_STATE_ICONIFIED | GDK_WINDOW_STATE_FULLSCREEN)) &&
            !(event->new_window_state & GDK_WINDOW_STATE_MAXIMIZED) &&
            !(event->new_window_state & GDK_WINDOW_STATE_ICONIFIED) &&
            !(event->new_window_state & GDK_WINDOW_STATE_FULLSCREEN))
        {
            instance->InvokeRestored();
        }

        return FALSE;
    }

    gboolean on_widget_deleted(GtkWidget* widget, GdkEvent* event, gpointer self)
    {
        if (PhotinoApplication::Instance().IsShuttingDown()) return FALSE;

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
} //namespace


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

    InitializeFromInitParams(initParams);

    platform_->sizeLimits.minWidth = (std::max)(0, initParams->MinWidth);
    platform_->sizeLimits.minHeight = (std::max)(0, initParams->MinHeight);
    platform_->sizeLimits.maxWidth = (std::max)(0, initParams->MaxWidth);
    platform_->sizeLimits.maxHeight = (std::max)(0, initParams->MaxHeight);

    if (platform_->sizeLimits.maxWidth > 0 && platform_->sizeLimits.minWidth > platform_->sizeLimits.maxWidth)    platform_->sizeLimits.maxWidth = platform_->sizeLimits.minWidth;
    if (platform_->sizeLimits.maxHeight > 0 && platform_->sizeLimits.minHeight > platform_->sizeLimits.maxHeight) platform_->sizeLimits.maxHeight = platform_->sizeLimits.minHeight;


    platform_->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    if (!platform_->window)
        std::abort();

    dialog_ = new PhotinoDialog();

    platform_->notifyInitialized = options_.notificationsEnabled && AcquireNotifications(options_.windowTitle);

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

        if (initParams->Width < 0) initParams->Width = -1;
        if (initParams->Height < 0) initParams->Height = -1;
        gtk_window_set_default_size(GTK_WINDOW(platform_->window), initParams->Width, initParams->Height);
    }

    SetMinSize(platform_->sizeLimits.minWidth, platform_->sizeLimits.minHeight);
    SetMaxSize(platform_->sizeLimits.maxWidth, platform_->sizeLimits.maxHeight);

    if (initParams->UseOsDefaultLocation)
        gtk_window_set_position(GTK_WINDOW(platform_->window), GTK_WIN_POS_NONE);
    else if (initParams->CenterOnInitialize)
        gtk_window_set_position(GTK_WINDOW(platform_->window), GTK_WIN_POS_CENTER);
    else
        gtk_window_move(GTK_WINDOW(platform_->window), initParams->Left, initParams->Top);

    SetTitle(options_.windowTitle);

    if (options_.chromeless)
        gtk_window_set_decorated(GTK_WINDOW(platform_->window), false);

    SetIconFile(options_.iconFileName);

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

    if (options_.transparentEnabled)
        SetTransparentEnabled(true);//visual/app-paintable

    if (!EnsureWebViewAttached())
        std::abort();

    const bool startMinimized = initParams->Minimized;
    const bool startMaximized = initParams->Maximized;
    const bool startFullScreen = options_.fullScreen;

    Show();

    if (options_.transparentEnabled)
        SetTransparentEnabled(true);//WebKit background alpha

    if (startMaximized)
        SetMaximized(true);

    if (startFullScreen)
        SetFullScreen(true);

    if (startMinimized)
        SetMinimized(true);

    g_signal_connect(G_OBJECT(platform_->window), "focus-in-event",
                     G_CALLBACK(on_focus_in_event),
                     this);

    g_signal_connect(G_OBJECT(platform_->window), "focus-out-event",
                     G_CALLBACK(on_focus_out_event),
                     this);

    if (options_.zoom != 100.0)
        SetZoom(options_.zoom);

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

    delete dialog_;
    dialog_ = nullptr;
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