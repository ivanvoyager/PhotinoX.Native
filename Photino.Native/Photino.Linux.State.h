#pragma once

#ifdef __linux__

#include "Photino.Geometry.h"
#include "Photino.Enums.h"

#include <gtk/gtk.h>

#include <vector>

namespace PhotinoX::Native
{
    struct ChromelessSettings
    {
        std::vector<LayoutRegion> DragRegions;
        std::vector<LayoutRegion> NoDragRegions;
        int ResizeBorderThickness = 8;
    };

    struct LinuxState
    {
        GtkWidget* window = nullptr;
        GtkWidget* webview = nullptr;
        GdkGeometry hints{};

        ChromelessSettings chromelessSettings;

        WindowSizeLimits sizeLimits;

        WindowGeometry lastGeometry;
        WindowGeometry normalGeometry;
        bool hasNormalGeometry = false;
        bool restoreNormalGeometryAfterUnmaximize = false;
        bool restoreNormalGeometryScheduled = false;

        bool isFullScreenTransitioning = false;
        bool isExitingFullScreen = false;
        PhotinoWindowState pendingStateAfterFullScreenExit = PhotinoWindowState::Normal;
        bool logicalMinimized = false;
    };
}

#endif