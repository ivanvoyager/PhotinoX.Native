#include "Photino.h"

#include <gtk/gtk.h>

#include <cassert>

using namespace PhotinoX::Native;

void Photino::Close() const
{
    assert(_window);
    if (!_window) return;

    gtk_window_close(GTK_WINDOW(_window));
}