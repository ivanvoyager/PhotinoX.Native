#include "Photino.h"
#include "Photino.Callbacks.h"
#include "Photino.Strings.h"

#include <gtk/gtk.h>

#include <cassert>

using namespace PhotinoX::Native;

void Photino::Close() const
{
    assert(_window);
    if (!_window) return;

    gtk_window_close(GTK_WINDOW(_window));
}

void Photino::SetTitle(const PlatformString& title)
{
    assert(_window);
    if (!_window) return;

    gtk_window_set_title(GTK_WINDOW(_window), title.c_str());
    _windowTitle = title;
}

void Photino::SetIconFile(const PlatformString& filename)
{
    assert(_window);
    if (!_window || filename.empty()) return;

    GError* error = nullptr;
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

void Photino::SetPosition(int x, int y)
{
    assert(_window);
    if (!_window) return;

    gtk_window_move(GTK_WINDOW(_window), x, y);
}

void Photino::GetSize(int* width, int* height) const
{
    assert(width || height);
    if (!width && !height) return;

    if (width) *width = 0;
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

void Photino::Center() const
{
    assert(_window);
    if (!_window) return;

    gint windowWidth = 0, windowHeight = 0;
    gtk_window_get_size(GTK_WINDOW(_window), &windowWidth, &windowHeight);

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

    GdkWindow* gdkWindow = gtk_widget_get_window(_window);
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

    gtk_window_move(GTK_WINDOW(_window),
                    screen.x + (screen.width - windowWidth) / 2,
                    screen.y + (screen.height - windowHeight) / 2);
}

void Photino::Restore() const
{
    assert(_window);
    if (!_window) return;

    gtk_window_present(GTK_WINDOW(_window));
}

unsigned int Photino::GetScreenDpi() const
{
    if (!_window) return 96;

    GdkScreen* screen = gtk_window_get_screen(GTK_WINDOW(_window));
    if (!screen) return 96;

    gdouble dpi = gdk_screen_get_resolution(screen);
    return dpi < 0 ? 96 : static_cast<unsigned int>(dpi);
}

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

void Photino::GetFullScreen(bool* fullScreen) const
{
    assert(fullScreen);
    if (!fullScreen) return;

    *fullScreen = _fullScreen;
}

void Photino::SetFullScreen(bool fullScreen)
{
    _fullScreen = fullScreen;

    assert(_window);
    if (!_window) return;

    if (fullScreen)
        gtk_window_fullscreen(GTK_WINDOW(_window));
    else
        gtk_window_unfullscreen(GTK_WINDOW(_window));
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

void Photino::SetMaximized(bool maximized)
{
    assert(_window);
    if (!_window) return;

    if (maximized)
        gtk_window_maximize(GTK_WINDOW(_window));
    else
        gtk_window_unmaximize(GTK_WINDOW(_window));
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

void Photino::SetMinimized(bool minimized)
{
    assert(_window);
    if (!_window) return;

    if (minimized)
        gtk_window_iconify(GTK_WINDOW(_window));
    else
        gtk_window_deiconify(GTK_WINDOW(_window));
}

void Photino::GetResizable(bool* resizable) const
{
    assert(resizable);
    if (!resizable) return;

    *resizable = false;

    if (!_window) return;

    *resizable = gtk_window_get_resizable(GTK_WINDOW(_window)) != FALSE;
}

void Photino::SetResizable(bool resizable)
{
    assert(_window);
    if (!_window) return;

    gtk_window_set_resizable(GTK_WINDOW(_window), resizable);
}

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

void Photino::SetTopmost(bool topmost)
{
    assert(_window);
    if (!_window) return;

    gtk_window_set_keep_above(GTK_WINDOW(_window), topmost);
}