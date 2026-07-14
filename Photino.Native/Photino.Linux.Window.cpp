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