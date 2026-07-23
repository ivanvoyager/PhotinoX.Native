#include "Photino.h"
#include "Photino.Enums.h"
#include "Photino.Callbacks.h"
#include "Photino.Linux.State.h"
#include "Photino.Strings.h"

#include <gtk/gtk.h>

#include <algorithm>
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
    options_.windowTitle = title;
}

void Photino::SetIconFile(const PlatformString& filename)
{
    assert(platform_->window);
    if (!platform_->window || filename.empty()) return;

    GError* error = nullptr;
    if (gtk_window_set_icon_from_file(GTK_WINDOW(platform_->window), filename.c_str(), &error))
    {
        options_.iconFileName = filename;
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

bool Photino::Activate() const
{
    assert(platform_->window);
    if (!platform_->window) return false;

    gtk_window_present(GTK_WINDOW(platform_->window));
    return true;
}

bool Photino::Center() const
{
    assert(platform_->window);
    if (!platform_->window) return false;

    gint windowWidth = 0;
    gint windowHeight = 0;
    gtk_window_get_size(GTK_WINDOW(platform_->window), &windowWidth, &windowHeight);

    GdkDisplay* display = gdk_display_get_default();
    if (display == nullptr)
        return false;

    GdkWindow* gdkWindow = gtk_widget_get_window(platform_->window);
    GdkMonitor* monitor = gdkWindow
                              ? gdk_display_get_monitor_at_window(display, gdkWindow)
                              : gdk_display_get_primary_monitor(display);

    if (monitor == nullptr)
    {
        monitor = gdk_display_get_monitor(display, 0);
        if (monitor == nullptr)
            return false;
    }

    GdkRectangle screen{};
    gdk_monitor_get_workarea(monitor, &screen);

    gtk_window_move(
        GTK_WINDOW(platform_->window),
        screen.x + (screen.width - windowWidth) / 2,
        screen.y + (screen.height - windowHeight) / 2);

    return true;
}

bool Photino::Maximize()
{
    assert(platform_->window);
    if (!platform_->window) return false;

    GtkWindow* window = GTK_WINDOW(platform_->window);

    if (IsMinimized())
        gtk_window_deiconify(window);

    if (IsFullScreen())
        gtk_window_unfullscreen(window);

    gtk_window_maximize(window);

    UpdateWindowState();

    return true;
}

bool Photino::Minimize()
{
    assert(platform_->window);
    if (!platform_->window) return false;

    GtkWindow* window = GTK_WINDOW(platform_->window);

    if (IsFullScreen())
        gtk_window_unfullscreen(window);

    gtk_window_iconify(window);

    UpdateWindowState();

    return true;
}

bool Photino::Restore()
{
    assert(platform_->window);
    if (!platform_->window) return false;

    GtkWindow* window = GTK_WINDOW(platform_->window);

    gtk_window_unfullscreen(window);
    gtk_window_unmaximize(window);
    gtk_window_deiconify(window);

    UpdateWindowState();

    return true;
}

bool Photino::Show() const
{
    assert(platform_->window);
    if (!platform_->window) return false;

    gtk_widget_show_all(platform_->window);
    return true;
}

void Photino::BeginWindowDrag() const
{
    // Not yet implemented on Linux. GTK offers gtk_window_begin_move_drag, but it
    // needs the button, root x/y and event time from the originating GDK event,
    // which this entry point does not receive. Left as a no-op until it can be
    // built and tested against GTK. Windows is the currently supported platform.
}

void Photino::BeginWindowResize(PhotinoWindowEdge) const
{
    // Not yet implemented on Linux. As with BeginWindowDrag, the GTK equivalent
    // (gtk_window_begin_resize_drag) needs the originating GDK event context.
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

bool Photino::IsFullScreen() const noexcept
{
    if (!platform_->window)
        return false;

    GdkWindow* gdkWindow = gtk_widget_get_window(GTK_WIDGET(platform_->window));
    if (!gdkWindow)
        return false;

    return (gdk_window_get_state(gdkWindow) & GDK_WINDOW_STATE_FULLSCREEN) != 0;
}

bool Photino::IsMinimized() const noexcept
{
    if (!platform_->window)
        return false;

    GdkWindow* gdkWindow = gtk_widget_get_window(GTK_WIDGET(platform_->window));
    if (!gdkWindow)
        return false;

    return (gdk_window_get_state(gdkWindow) & GDK_WINDOW_STATE_ICONIFIED) != 0;
}

bool Photino::IsMaximized() const noexcept
{
    if (!platform_->window)
        return false;

    GdkWindow* gdkWindow = gtk_widget_get_window(GTK_WIDGET(platform_->window));
    if (!gdkWindow)
        return false;

    return (gdk_window_get_state(gdkWindow) & GDK_WINDOW_STATE_MAXIMIZED) != 0;
}

PhotinoWindowState Photino::GetPlatformWindowState() const noexcept
{
    if (!platform_->window)
        return options_.windowState;

    GdkWindow* gdkWindow = gtk_widget_get_window(GTK_WIDGET(platform_->window));
    if (!gdkWindow)
        return options_.windowState;

    const GdkWindowState state = gdk_window_get_state(gdkWindow);

    if (state & GDK_WINDOW_STATE_ICONIFIED)
        return PhotinoWindowState::Minimized;

    if (state & GDK_WINDOW_STATE_FULLSCREEN)
        return PhotinoWindowState::FullScreen;

    if (state & GDK_WINDOW_STATE_MAXIMIZED)
        return PhotinoWindowState::Maximized;

    return PhotinoWindowState::Normal;
}

void Photino::SetFullScreen(bool fullScreen)
{
    assert(platform_->window);
    if (!platform_->window) return;

    GtkWindow* window = GTK_WINDOW(platform_->window);

    if (fullScreen)
    {
        if (IsMinimized())
            gtk_window_deiconify(window);

        gtk_window_fullscreen(window);
    }
    else
    {
        gtk_window_unfullscreen(window);
    }

    UpdateWindowState();
}

void Photino::SetMaximized(bool maximized)
{
    assert(platform_->window);
    if (!platform_->window) return;

    if (maximized)
    {
        Maximize();
        return;
    }

    gtk_window_unmaximize(GTK_WINDOW(platform_->window));
    UpdateWindowState();
}

void Photino::SetMinimized(bool minimized)
{
    assert(platform_->window);
    if (!platform_->window) return;

    if (minimized)
    {
        Minimize();
        return;
    }

    gtk_window_deiconify(GTK_WINDOW(platform_->window));
    UpdateWindowState();
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
    GdkWindow* gdkWindow = gtk_widget_get_window(GTK_WIDGET(platform_->window));
    if (!gdkWindow) return;

    GdkWindowState flags = gdk_window_get_state(gdkWindow);
    *topmost = (flags & GDK_WINDOW_STATE_ABOVE) != 0;
}

void Photino::SetTopmost(bool topmost)
{
    assert(platform_->window);
    if (!platform_->window) return;

    gtk_window_set_keep_above(GTK_WINDOW(platform_->window), topmost);
}