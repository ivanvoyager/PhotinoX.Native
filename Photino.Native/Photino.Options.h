#pragma once

#include "Photino.Strings.h"
#include "Photino.Enums.h"

namespace PhotinoX::Native
{
struct PhotinoOptions
{
    PlatformString windowTitle;
    PlatformString iconFileName;

    PlatformString startString;
    PlatformString startUrl;

    PlatformString userDataFolder; // Currently supported only on Windows.
    PlatformString userAgent;
    PlatformString browserControlInitParameters;
    PlatformString notificationRegistrationId; // TODO: Currently supported only on Windows.

    bool transparentEnabled = false;
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
    bool notificationsEnabled = true;
    bool contextMenuEnabled = true;                 // TODO: Currently supported only on Windows & Linux.
    bool zoomEnabled = true;                        // TODO: Currently supported only on Windows

    int zoom = 100;
    bool chromeless = false;
    bool resizable = true;

    bool useNativeWindowOwner = false;

    PhotinoWindowState windowState = PhotinoWindowState::Normal;
};
} // namespace PhotinoX::Native