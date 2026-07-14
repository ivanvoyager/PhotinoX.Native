#include "Photino.h"
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