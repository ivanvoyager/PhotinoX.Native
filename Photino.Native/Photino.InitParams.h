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
        ContentLoadedCallback ContentLoadedHandler;             // #20
        WebResourceRequestedCallback CustomSchemeHandler;       // #21
        ClosedCallback ClosedHandler;                           // #22
        FullScreenChangedCallback FullScreenChangedHandler;     // #23
        StateChangedCallback StateChangedHandler;               // #24

        int Left;                                               // #25
        int Top;                                                // #26
        int Width;                                              // #27
        int Height;                                             // #28
        int Zoom;                                               // #29
        int MinWidth;                                           // #30
        int MinHeight;                                          // #31
        int MaxWidth;                                           // #32
        int MaxHeight;                                          // #33

        PhotinoWindowState WindowState;                         // #34

        bool CenterOnInitialize;                                // #35
        bool Chromeless;                                        // #36
        bool Transparent;                                       // #37
        bool ContextMenuEnabled;                                // #38
        bool ZoomEnabled;                                       // #39
        bool DevToolsEnabled;                                   // #40
        bool Resizable;                                         // #41
        bool Topmost;                                           // #42
        bool UseOsDefaultLocation;                              // #43
        bool UseOsDefaultSize;                                  // #44
        bool GrantBrowserPermissions;                           // #45
        bool MediaAutoplayEnabled;                              // #46
        bool FileSystemAccessEnabled;                           // #47
        bool WebSecurityEnabled;                                // #48
        bool JavascriptClipboardAccessEnabled;                  // #49
        bool MediaStreamEnabled;                                // #50
        bool SmoothScrollingEnabled;                            // #51
        bool IgnoreCertificateErrorsEnabled;                    // #52
        bool NotificationsEnabled;                              // #53

        bool UseNativeWindowOwner;                              // #53

        int ChromelessDragRegionHeight;                         // #54
        int ChromelessDragRegionLeftInset;                      // #55
        int ChromelessDragRegionRightInset;                     // #56
        int ChromelessResizeBorderThickness;                    // #57

        int Size;                                               // #58
    };

    static_assert(std::is_standard_layout_v<PhotinoInitParams>,
        "PhotinoInitParams must remain standard-layout for managed/native interop.");

    static_assert(sizeof(PhotinoWindowState) == sizeof(int),
        "PhotinoWindowState must remain int-sized for managed/native interop.");

    static_assert(sizeof(PhotinoInitParams) == 392,
        "PhotinoInitParams size changed. Update the managed ABI layout and size validation.");
}