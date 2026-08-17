#pragma once

#include "Photino.Strings.h"
#include "Photino.Enums.h"

namespace PhotinoX::Native
{
    struct PhotinoOptions
    {
        // Window
        PlatformString windowTitle;
        PlatformString iconFileName;

        bool chromeless = false;
        bool transparentEnabled = false;

        // Geometry
        PhotinoWindowState windowState = PhotinoWindowState::Normal;
        bool resizable = true;

        // Browser
        PlatformString startString;
        PlatformString startUrl;

        PlatformString userAgent;
        PlatformString browserControlInitParameters;
        PlatformString userDataFolder; // Currently supported only on Windows.

        int zoom = 100;
        bool zoomEnabled = true;        // TODO: Currently supported only on Windows
        bool contextMenuEnabled = true; // TODO: Currently supported only on Windows & Linux.
        bool statusBarEnabled = true;   // TODO: Currently supported only on Windows
        bool devToolsEnabled = true;

        bool grantBrowserPermissions = true;
    #if defined(_WIN32) || defined(__linux__)
        bool mediaAutoplayEnabled = true;
    #endif
        bool fileSystemAccessEnabled = true;
        bool webSecurityEnabled = true;                 // TODO: Currently supported only on Windows & Linux.
        bool javascriptClipboardAccessEnabled = true;   // TODO: Currently supported only on Windows & Linux.
        bool mediaStreamEnabled = true;                 // TODO: Currently supported only on Windows & Linux.
    #if defined(_WIN32) || defined(__linux__)
        bool smoothScrollingEnabled = true;
    #endif
        bool ignoreCertificateErrorsEnabled = false;
    };
} // namespace PhotinoX::Native