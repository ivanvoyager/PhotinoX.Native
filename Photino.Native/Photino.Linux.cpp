#include "Photino.h"
#include "Photino.Dialog.h"
#include "Photino.Strings.h"
#include "Photino.Application.h"
#include "Photino.Linux.State.h"
#include "Photino.Linux.Debug.h"

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <memory>
#include <mutex>

#include <X11/Xlib.h>
#include <gtk/gtk.h>

#ifdef __GLIBC__
#include <gnu/libc-version.h>
#endif

using namespace PhotinoX::Native;

namespace
{
    gboolean on_configure_event(GtkWidget* widget, GdkEvent* event, gpointer self)
    {
        if (!event) return FALSE;

        if (event->type == GDK_CONFIGURE)
        {
            auto instance = static_cast<Photino*>(self);
            if (!instance) return FALSE;

            PHOTINO_LINUX_LOG(
                "[linux-event] configure x=%d y=%d width=%d height=%d\n",
                event->configure.x,
                event->configure.y,
                event->configure.width,
                event->configure.height);

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

        PHOTINO_LINUX_LOG(
            "[linux-event] window-state changed=0x%x new=0x%x\n",
            static_cast<unsigned int>(event->changed_mask),
            static_cast<unsigned int>(event->new_window_state));

        if (event->changed_mask & (GDK_WINDOW_STATE_FULLSCREEN |
                                   GDK_WINDOW_STATE_MAXIMIZED |
                                   GDK_WINDOW_STATE_ICONIFIED |
                                   GDK_WINDOW_STATE_FOCUSED))
        {
            instance->HandleWindowStateEvent();
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

    void on_webview_size_allocate(GtkWidget* widget, GdkRectangle* allocation, gpointer self)
    {
        auto instance = static_cast<Photino*>(self);

        if (!instance || !widget || !allocation)
            return;

        instance->UpdateWebViewInputShape();
    }

    bool IsInChromelessDragRegion(Photino* instance, GtkWidget* widget, GdkEventButton* event)
    {
        if (!instance || !widget || !event)
            return false;

        auto& platform = instance->Platform();

        const int width = gtk_widget_get_allocated_width(widget);
        const int x = static_cast<int>(event->x);
        const int y = static_cast<int>(event->y);

        const int resizeBorder = instance->CanBeginResize()
            ? platform.chromelessSettings.ResizeBorderThickness
            : 0;

        if (y < resizeBorder || y >= platform.chromelessSettings.DragRegionHeight)
            return false;

        const int leftInset = (std::clamp)(platform.chromelessSettings.DragRegionLeftInset, 0, width);
        const int rightInset = (std::clamp)(platform.chromelessSettings.DragRegionRightInset, 0, width);
        const int dragRight = (std::max)(leftInset, width - rightInset);

        return x >= leftInset && x < dragRight;
    }

    gboolean on_webview_button_press_event(GtkWidget* widget, GdkEventButton* event, gpointer self)
    {
        auto instance = static_cast<Photino*>(self);

        if (!instance || !widget || !event)
            return FALSE;

        if (event->button != GDK_BUTTON_PRIMARY)
            return FALSE;

        if (!IsInChromelessDragRegion(instance, widget, event))
            return FALSE;

        if (event->type == GDK_2BUTTON_PRESS)
        {
            if (instance->IsMaximized())
                instance->Restore();
            else
                instance->Maximize();

            return TRUE;
        }

        if (!instance->CanBeginDrag())
            return FALSE;

        auto& platform = instance->Platform();

        gtk_window_begin_move_drag(
            GTK_WINDOW(platform.window),
            event->button,
            static_cast<int>(event->x_root),
            static_cast<int>(event->y_root),
            event->time);

        return TRUE;
    }

    int GetResizeEdge(int x, int y, int width, int height, int border)
    {
        const bool top = y < border;
        const bool bottom = y >= height - border;
        const bool left = x < border;
        const bool right = x >= width - border;

        if (top && left) return GDK_WINDOW_EDGE_NORTH_WEST;
        if (top && right) return GDK_WINDOW_EDGE_NORTH_EAST;
        if (bottom && left) return GDK_WINDOW_EDGE_SOUTH_WEST;
        if (bottom && right) return GDK_WINDOW_EDGE_SOUTH_EAST;
        if (top) return GDK_WINDOW_EDGE_NORTH;
        if (bottom) return GDK_WINDOW_EDGE_SOUTH;
        if (left) return GDK_WINDOW_EDGE_WEST;
        if (right) return GDK_WINDOW_EDGE_EAST;

        return -1;
    }

    const char* GetResizeCursorName(int edge)
    {
        switch (edge)
        {
        case GDK_WINDOW_EDGE_NORTH: return "n-resize";
        case GDK_WINDOW_EDGE_SOUTH: return "s-resize";
        case GDK_WINDOW_EDGE_WEST: return "w-resize";
        case GDK_WINDOW_EDGE_EAST: return "e-resize";
        case GDK_WINDOW_EDGE_NORTH_WEST: return "nw-resize";
        case GDK_WINDOW_EDGE_NORTH_EAST: return "ne-resize";
        case GDK_WINDOW_EDGE_SOUTH_WEST: return "sw-resize";
        case GDK_WINDOW_EDGE_SOUTH_EAST: return "se-resize";
        default: return nullptr;
        }
    }

    gboolean on_button_press_event(GtkWidget* widget, GdkEventButton* event, gpointer self)
    {
        auto instance = static_cast<Photino*>(self);

        if (!instance || !widget || !event)
            return FALSE;

        if (event->button != GDK_BUTTON_PRIMARY)
            return FALSE;

        if (!instance->CanBeginResize())
            return FALSE;

        const int width = gtk_widget_get_allocated_width(widget);
        const int height = gtk_widget_get_allocated_height(widget);

        auto& platform = instance->Platform();

        const int edge = GetResizeEdge(static_cast<int>(event->x), static_cast<int>(event->y), width, height, platform.chromelessSettings.ResizeBorderThickness);

        if (edge < 0)
            return FALSE;

        gtk_window_begin_resize_drag(GTK_WINDOW(widget),
            static_cast<GdkWindowEdge>(edge), event->button,
            static_cast<int>(event->x_root),
            static_cast<int>(event->y_root),
            event->time);

        return TRUE;
    }

    gboolean on_motion_notify_event(GtkWidget* widget, GdkEventMotion* event, gpointer self)
    {
        auto instance = static_cast<Photino*>(self);

        if (!instance || !widget || !event)
            return FALSE;

        auto gdkWindow = gtk_widget_get_window(widget);
        if (!gdkWindow)
            return FALSE;

        if (!instance->CanBeginResize())
        {
            gdk_window_set_cursor(gdkWindow, nullptr);
            return FALSE;
        }

        const int width = gtk_widget_get_allocated_width(widget);
        const int height = gtk_widget_get_allocated_height(widget);

        auto& platform = instance->Platform();

        const int edge = GetResizeEdge(
            static_cast<int>(event->x),
            static_cast<int>(event->y),
            width,
            height,
            platform.chromelessSettings.ResizeBorderThickness);

        const char* cursorName = GetResizeCursorName(edge);
        if (!cursorName)
        {
            gdk_window_set_cursor(gdkWindow, nullptr);
            return FALSE;
        }

        auto display = gdk_window_get_display(gdkWindow);
        auto cursor = gdk_cursor_new_from_name(display, cursorName);

        gdk_window_set_cursor(gdkWindow, cursor);

        if (cursor)
            g_object_unref(cursor);

        return FALSE;
    }

    gboolean on_leave_notify_event(GtkWidget* widget, GdkEventCrossing* event, gpointer self)
    {
        if (!widget)
            return FALSE;

        auto gdkWindow = gtk_widget_get_window(widget);
        if (gdkWindow)
            gdk_window_set_cursor(gdkWindow, nullptr);

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

    if (initParams->Size != sizeof(PhotinoInitParams) ||
        initParams->AbiVersion != PhotinoInitParams::NativeAbiVersion)
    {
        GtkWidget* dialog = gtk_message_dialog_new(
            nullptr,
            GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_CLOSE,
            "Window initial parameters ABI mismatch. Passed size: %i bytes, expected size: %zu bytes. Passed ABI version: %i, expected ABI version: %i.",
            initParams->Size,
            sizeof(PhotinoInitParams),
            initParams->AbiVersion,
            PhotinoInitParams::NativeAbiVersion);

        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        std::abort();
    }

    InitializeFromInitParams(initParams);

    const auto startupWindowState = options_.windowState;
    options_.windowState = PhotinoWindowState::Normal;

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
    {
        gtk_window_set_decorated(GTK_WINDOW(platform_->window), FALSE);

        platform_->chromelessSettings.DragRegionHeight = initParams->ChromelessDragRegionHeight;
        platform_->chromelessSettings.DragRegionLeftInset = initParams->ChromelessDragRegionLeftInset;
        platform_->chromelessSettings.DragRegionRightInset = initParams->ChromelessDragRegionRightInset;
        platform_->chromelessSettings.ResizeBorderThickness = initParams->ChromelessResizeBorderThickness;
    }

    SetIconFile(options_.iconFileName);

    if (!initParams->Resizable)
        SetResizable(false);

    if (initParams->Topmost)
        SetTopmost(true);

    // g_signal_connect(platform_->window, "size-allocate",
    //	G_CALLBACK(on_size_allocate),
    //	this);

    g_signal_connect(platform_->window, "configure-event",    G_CALLBACK(on_configure_event), this);
    g_signal_connect(platform_->window, "window-state-event", G_CALLBACK(on_window_state_event), this);
    g_signal_connect(platform_->window, "delete-event", G_CALLBACK(on_widget_deleted), this);
    g_signal_connect(platform_->window, "destroy", G_CALLBACK(on_widget_destroyed), this);

    if (options_.transparentEnabled)
        SetTransparentEnabled(true);//visual/app-paintable

    if (!EnsureWebViewAttached())
        std::abort();

    if (options_.chromeless)
    {
        gtk_widget_add_events(platform_->window, GDK_POINTER_MOTION_MASK | GDK_BUTTON_PRESS_MASK | GDK_LEAVE_NOTIFY_MASK);
        gtk_widget_add_events(platform_->webview, GDK_BUTTON_PRESS_MASK);

        g_signal_connect(platform_->window, "motion-notify-event", G_CALLBACK(on_motion_notify_event), this);
        g_signal_connect(platform_->window, "button-press-event", G_CALLBACK(on_button_press_event), this);
        g_signal_connect(platform_->window, "leave-notify-event", G_CALLBACK(on_leave_notify_event), this);

        g_signal_connect(platform_->webview, "button-press-event", G_CALLBACK(on_webview_button_press_event), this);
        g_signal_connect(platform_->webview, "size-allocate", G_CALLBACK(on_webview_size_allocate), this);
    }

    suppressWindowStateCallbacks_ = true;

    Show();
    UpdateWindowState();

    if (options_.transparentEnabled)
        SetTransparentEnabled(true); // WebKit background alpha

    switch (startupWindowState)
    {
    case PhotinoWindowState::Maximized:
        SetMaximized(true);
        break;
    case PhotinoWindowState::Minimized:
        SetMinimized(true);
        break;
    case PhotinoWindowState::FullScreen:
        SetFullScreen(true);
        break;
    default:
        break;
    }

    UpdateWindowState();
    suppressWindowStateCallbacks_ = false;

    g_signal_connect(platform_->window, "focus-in-event", G_CALLBACK(on_focus_in_event), this);
    g_signal_connect(platform_->window, "focus-out-event", G_CALLBACK(on_focus_out_event), this);

    if (options_.zoom != 100.0)
        SetZoom(options_.zoom);
}

Photino::~Photino()
{
    delete dialog_;
    dialog_ = nullptr;
}

const char* Photino::GetGtkVersion()
{
    static const std::string version =
        std::to_string(gtk_get_major_version()) + "." +
        std::to_string(gtk_get_minor_version()) + "." +
        std::to_string(gtk_get_micro_version());

    return version.c_str();
}

const char* Photino::GetGlibcVersion()
{
#ifdef __GLIBC__
    return gnu_get_libc_version();
#else
    return nullptr;
#endif
}