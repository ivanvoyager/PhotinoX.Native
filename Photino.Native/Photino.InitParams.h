#pragma once

#include "Photino.Strings.h"
#include "Photino.Callbacks.h"
#include "Photino.Enums.h"

#include <type_traits>

namespace PhotinoX::Native
{
    class Photino;

    inline constexpr int MaxCustomSchemeNames = 16;

    struct PhotinoInitParams
    {
        Utf8String StartString;                                 // #1
        Utf8String StartUrl;                                    // #2
        Utf8String Title;                                       // #3
        Utf8String WindowIconFile;                              // #4
        Utf8String TemporaryFilesPath;                          // #5
        Utf8String UserAgent;                                   // #6
        Utf8String BrowserControlInitParameters;                // #7
        Utf8String NotificationRegistrationId;                  // #8
        Utf8String CustomSchemeNames[MaxCustomSchemeNames];     // #9

        Photino* ParentInstance;                                // #10

        ClosingCallback ClosingHandler;                         // #11
        FocusInCallback FocusInHandler;                         // #12
        FocusOutCallback FocusOutHandler;                       // #13
        ResizedCallback ResizedHandler;                         // #14
        MaximizedCallback MaximizedHandler;                     // #15
        RestoredCallback RestoredHandler;                       // #16
        MinimizedCallback MinimizedHandler;                     // #17
        MovedCallback MovedHandler;                             // #18
        WebMessageReceivedCallback WebMessageReceivedHandler;   // #19
        WebResourceRequestedCallback CustomSchemeHandler;       // #20
        ClosedCallback ClosedHandler;                           // #21
        FullScreenChangedCallback FullScreenChangedHandler;     // #22
        StateChangedCallback StateChangedHandler;               // #23

        int Left;                                               // #24
        int Top;                                                // #25
        int Width;                                              // #26
        int Height;                                             // #27
        int Zoom;                                               // #28
        int MinWidth;                                           // #29
        int MinHeight;                                          // #30
        int MaxWidth;                                           // #31
        int MaxHeight;                                          // #32

        PhotinoWindowState WindowState;                         // #33

        bool CenterOnInitialize;                                // #34
        bool Chromeless;                                        // #35
        bool Transparent;                                       // #36
        bool ContextMenuEnabled;                                // #37
        bool ZoomEnabled;                                       // #38
        bool DevToolsEnabled;                                   // #39
        bool Resizable;                                         // #40
        bool Topmost;                                           // #41
        bool UseOsDefaultLocation;                              // #42
        bool UseOsDefaultSize;                                  // #43
        bool GrantBrowserPermissions;                           // #44
        bool MediaAutoplayEnabled;                              // #45
        bool FileSystemAccessEnabled;                           // #46
        bool WebSecurityEnabled;                                // #47
        bool JavascriptClipboardAccessEnabled;                  // #48
        bool MediaStreamEnabled;                                // #49
        bool SmoothScrollingEnabled;                            // #50
        bool IgnoreCertificateErrorsEnabled;                    // #51
        bool NotificationsEnabled;                              // #52

        bool UseNativeWindowOwner;                              // #53

        int Size;                                               // #54
    };

    static_assert(std::is_standard_layout_v<PhotinoInitParams>,
        "PhotinoInitParams must remain standard-layout for managed/native interop.");

    static_assert(sizeof(PhotinoWindowState) == sizeof(int),
        "PhotinoWindowState must remain int-sized for managed/native interop.");

    static_assert(sizeof(PhotinoInitParams) == 368,
        "PhotinoInitParams size changed. Update the managed ABI layout and size validation.");
}