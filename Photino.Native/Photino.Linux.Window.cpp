#include "Photino.h"
#include "Photino.Callbacks.h"
#include "Photino.Linux.State.h"
#include "Photino.Strings.h"

#include <gtk/gtk.h>

#include <cassert>

using namespace PhotinoX::Native;

void* Photino::GetGtkWidget() const noexcept { return platform_->window; }

void Photino::Close() const
{
    assert(platform_->window);
    if (!platform_->window) return;

    gtk_window_close(GTK_WINDOW(platform_->window));
}

void Photino::SetTitle(const PlatformString& title)
{
    assert(platform_->window);
    if (!platform_->window) return;

    gtk_window_set_title(GTK_WINDOW(platform_->window), title.c_str());
    windowTitle_ = title;
}

void Photino::SetIconFile(const PlatformString& filename)
{
    assert(platform_->window);
    if (!platform_->window || filename.empty()) return;

    GError* error = nullptr;
    if (gtk_window_set_icon_from_file(GTK_WINDOW(platform_->window), filename.c_str(), &error))
    {
        iconFileName_ = filename;
        return;
    }

    if (error)
    {
        g_warning("Failed to set window icon: %s", error->message);
        g_error_free(error);
    }
}

void Photino::GetPosition(int* x, int* y) const
{
    assert(x || y);
    if (!x && !y) return;

    if (x) *x = 0;
    if (y) *y = 0;

    if (!platform_->window) return;

    gint windowX = 0;
    gint windowY = 0;
    gtk_window_get_position(GTK_WINDOW(platform_->window), &windowX, &windowY);

    if (x) *x = windowX;
    if (y) *y = windowY;
}

void Photino::SetPosition(int x, int y)
{
    assert(platform_->window);
    if (!platform_->window) return;

    gtk_window_move(GTK_WINDOW(platform_->window), x, y);
}

void Photino::GetSize(int* width, int* height) const
{
    assert(width || height);
    if (!width && !height) return;

    if (width) *width = 0;
    if (height) *height = 0;

    if (!platform_->window) return;

    gint windowWidth = 0;
    gint windowHeight = 0;
    gtk_window_get_size(GTK_WINDOW(platform_->window), &windowWidth, &windowHeight);

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

void Photino::SetSize(int width, int height)
{
    assert(platform_->window);
    if (!platform_->window) return;

    if (width <= 0 || height <= 0) return;

    int newWidth = width;
    int newHeight = height;

    if (platform_->sizeLimits.minWidth > 0 && newWidth < platform_->sizeLimits.minWidth)    newWidth = platform_->sizeLimits.minWidth;
    if (platform_->sizeLimits.minHeight > 0 && newHeight < platform_->sizeLimits.minHeight) newHeight = platform_->sizeLimits.minHeight;
    if (platform_->sizeLimits.maxWidth > 0 && newWidth > platform_->sizeLimits.maxWidth)    newWidth = platform_->sizeLimits.maxWidth;
    if (platform_->sizeLimits.maxHeight > 0 && newHeight > platform_->sizeLimits.maxHeight) newHeight = platform_->sizeLimits.maxHeight;

    gtk_window_resize(GTK_WINDOW(platform_->window), newWidth, newHeight);
}

void Photino::SetMinSize(int width, int height)
{
    platform_->sizeLimits.minWidth = (std::max)(0, width);
    platform_->sizeLimits.minHeight = (std::max)(0, height);

    if (platform_->sizeLimits.maxWidth > 0 && platform_->sizeLimits.minWidth > platform_->sizeLimits.maxWidth)
        platform_->sizeLimits.maxWidth = platform_->sizeLimits.minWidth;

    if (platform_->sizeLimits.maxHeight > 0 && platform_->sizeLimits.minHeight > platform_->sizeLimits.maxHeight)
        platform_->sizeLimits.maxHeight = platform_->sizeLimits.minHeight;

    ApplyGeometryHints();
}

void Photino::SetMaxSize(int width, int height)
{

    platform_->sizeLimits.maxWidth = (std::max)(0, width);
    platform_->sizeLimits.maxHeight = (std::max)(0, height);

    if (platform_->sizeLimits.maxWidth > 0 && platform_->sizeLimits.maxWidth < platform_->sizeLimits.minWidth)
        platform_->sizeLimits.minWidth = platform_->sizeLimits.maxWidth;

    if (platform_->sizeLimits.maxHeight > 0 && platform_->sizeLimits.maxHeight < platform_->sizeLimits.minHeight)
        platform_->sizeLimits.minHeight = platform_->sizeLimits.maxHeight;

    ApplyGeometryHints();
}

void Photino::ApplyGeometryHints()
{
    if (!platform_->window) return;

    platform_->hints.min_width = platform_->sizeLimits.minWidth;
    platform_->hints.min_height = platform_->sizeLimits.minHeight;
    platform_->hints.max_width = platform_->sizeLimits.maxWidth > 0 ? platform_->sizeLimits.maxWidth : G_MAXINT;
    platform_->hints.max_height = platform_->sizeLimits.maxHeight > 0 ? platform_->sizeLimits.maxHeight : G_MAXINT;

    gtk_window_set_geometry_hints(
        GTK_WINDOW(platform_->window),
        nullptr,
        &platform_->hints,
        static_cast<GdkWindowHints>(GDK_HINT_MIN_SIZE | GDK_HINT_MAX_SIZE));
}

void Photino::Center() const
{
    assert(platform_->window);
    if (!platform_->window) return;

    gint windowWidth = 0, windowHeight = 0;
    gtk_window_get_size(GTK_WINDOW(platform_->window), &windowWidth, &windowHeight);

    GdkRectangle screen = {0};

    GdkDisplay* display = gdk_display_get_default();
    if (display == NULL)
    {
        GtkWidget* dialog = gtk_message_dialog_new(
            nullptr, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "gdk_display_get_default() returned NULL");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        return;
    }

    GdkWindow* gdkWindow = gtk_widget_get_window(platform_->window);
    GdkMonitor* monitor = gdkWindow
                              ? gdk_display_get_monitor_at_window(display, gdkWindow)
                              : gdk_display_get_primary_monitor(display);

    if (monitor == nullptr)
    {
        monitor = gdk_display_get_monitor(display, 0); // Attempt to get the first monitor
        if (monitor == nullptr)
        {
            GtkWidget* dialog = gtk_message_dialog_new(
                nullptr, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "gdk_display_get_monitor() returned NULL");
            gtk_dialog_run(GTK_DIALOG(dialog));
            gtk_widget_destroy(dialog);
            return;
        }
    }

    gdk_monitor_get_workarea(monitor, &screen);

    gtk_window_move(GTK_WINDOW(platform_->window),
                    screen.x + (screen.width - windowWidth) / 2,
                    screen.y + (screen.height - windowHeight) / 2);
}

void Photino::Restore() const
{
    assert(platform_->window);
    if (!platform_->window) return;

    gtk_window_present(GTK_WINDOW(platform_->window));
}

unsigned int Photino::GetScreenDpi() const
{
    if (!platform_->window) return 96;

    GdkScreen* screen = gtk_window_get_screen(GTK_WINDOW(platform_->window));
    if (!screen) return 96;

    gdouble dpi = gdk_screen_get_resolution(screen);
    return dpi < 0 ? 96 : static_cast<unsigned int>(dpi);
}

void Photino::GetAllMonitors(GetAllMonitorsCallback callback) const noexcept
{
    assert(callback);
    if (!callback) return;

    assert(platform_->window);
    if (!platform_->window) return;

    GdkScreen* screen = gtk_window_get_screen(GTK_WINDOW(platform_->window));
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

void Photino::GetFullScreen(bool* fullScreen) const
{
    assert(fullScreen);
    if (!fullScreen) return;

    *fullScreen = fullScreen_;
}

void Photino::SetFullScreen(bool fullScreen)
{
    fullScreen_ = fullScreen;

    assert(platform_->window);
    if (!platform_->window) return;

    if (fullScreen)
        gtk_window_fullscreen(GTK_WINDOW(platform_->window));
    else
        gtk_window_unfullscreen(GTK_WINDOW(platform_->window));
}

void Photino::GetMaximized(bool* isMaximized) const
{
    assert(isMaximized);
    if (!isMaximized) return;

    *isMaximized = false;

    if (!platform_->window) return;

    // gboolean maximized = gtk_window_is_maximized(GTK_WINDOW(platform_->window));  //this method doesn't work
    //*isMaximized = maximized;
    GdkWindow* gdk_window = gtk_widget_get_window(GTK_WIDGET(platform_->window));
    if (!gdk_window)
        return;

    GdkWindowState flags = gdk_window_get_state(gdk_window);
    *isMaximized = (flags & GDK_WINDOW_STATE_MAXIMIZED) != 0;
}

void Photino::SetMaximized(bool maximized)
{
    assert(platform_->window);
    if (!platform_->window) return;

    if (maximized)
        gtk_window_maximize(GTK_WINDOW(platform_->window));
    else
        gtk_window_unmaximize(GTK_WINDOW(platform_->window));
}

void Photino::GetMinimized(bool* isMinimized) const
{
    assert(isMinimized);
    if (!isMinimized) return;

    *isMinimized = false;

    if (!platform_->window) return;

    GdkWindow* gdk_window = gtk_widget_get_window(GTK_WIDGET(platform_->window));
    if (gdk_window == NULL)
        return;

    GdkWindowState flags = gdk_window_get_state(gdk_window);
    *isMinimized = (flags & GDK_WINDOW_STATE_ICONIFIED) != 0;
}

void Photino::SetMinimized(bool minimized)
{
    assert(platform_->window);
    if (!platform_->window) return;

    if (minimized)
        gtk_window_iconify(GTK_WINDOW(platform_->window));
    else
        gtk_window_deiconify(GTK_WINDOW(platform_->window));
}

void Photino::GetResizable(bool* resizable) const
{
    assert(resizable);
    if (!resizable) return;

    *resizable = false;

    if (!platform_->window) return;

    *resizable = gtk_window_get_resizable(GTK_WINDOW(platform_->window)) != FALSE;
}

void Photino::SetResizable(bool resizable)
{
    assert(platform_->window);
    if (!platform_->window) return;

    gtk_window_set_resizable(GTK_WINDOW(platform_->window), resizable);
}

void Photino::GetTopmost(bool* topmost) const
{
    assert(topmost);
    if (!topmost) return;

    *topmost = false;

    if (!platform_->window) return;

    // TODO: This flag is not set in GDK3. WebKit does not support GTK5 yet.
    GdkWindow* gdk_window = gtk_widget_get_window(GTK_WIDGET(platform_->window));
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

void Photino::SetTopmost(bool topmost)
{
    assert(platform_->window);
    if (!platform_->window) return;

    gtk_window_set_keep_above(GTK_WINDOW(platform_->window), topmost);
}