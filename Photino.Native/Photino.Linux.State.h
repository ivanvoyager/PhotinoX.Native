#pragma once

#ifdef __linux__

#include "Photino.Geometry.h"

#include <gtk/gtk.h>

namespace PhotinoX::Native
{
    struct LinuxState
    {
        GtkWidget* window = nullptr;
        GtkWidget* webview = nullptr;
        GdkGeometry hints{};

        WindowGeometry lastGeometry;
        WindowSizeLimits sizeLimits;

        bool notifyInitialized = false;
    };
}

#endif