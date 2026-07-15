#pragma once

#ifdef __linux__

#include <gtk/gtk.h>

namespace PhotinoX::Native
{
    struct LinuxState
    {
        GtkWidget* window = nullptr;
        GtkWidget* webview = nullptr;
        GdkGeometry hints{};
    };
}

#endif