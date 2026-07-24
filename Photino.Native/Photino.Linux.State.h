#pragma once

#ifdef __linux__

#include "Photino.Geometry.h"
#include "Photino.Enums.h"

#include <gtk/gtk.h>

namespace PhotinoX::Native
{
    struct LinuxState
    {
        GtkWidget* window = nullptr;
        GtkWidget* webview = nullptr;
        GdkGeometry hints{};

        WindowSizeLimits sizeLimits;

        WindowGeometry lastGeometry;
        WindowGeometry normalGeometry;
        bool hasNormalGeometry = false;
        bool restoreNormalGeometryAfterUnmaximize = false;
        bool restoreNormalGeometryScheduled = false;

        bool notifyInitialized = false;

        bool isFullScreenTransitioning = false;
        bool isExitingFullScreen = false;
        PhotinoWindowState pendingStateAfterFullScreenExit = PhotinoWindowState::Normal;
        bool logicalMinimized = false;
    };
}

#endif