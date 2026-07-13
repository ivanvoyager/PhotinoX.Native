#pragma once

#include "Photino.Strings.h"
#include "Photino.Callbacks.h"

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

        Photino *ParentInstance;                                // #10

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

        int Left;                                               // #22
        int Top;                                                // #23
        int Width;                                              // #24
        int Height;                                             // #25
        int Zoom;                                               // #26
        int MinWidth;                                           // #27
        int MinHeight;                                          // #28
        int MaxWidth;                                           // #29
        int MaxHeight;                                          // #30

        bool CenterOnInitialize;                                // #31
        bool Chromeless;                                        // #32
        bool Transparent;                                       // #33
        bool ContextMenuEnabled;                                // #34
        bool ZoomEnabled;                                       // #35
        bool DevToolsEnabled;                                   // #36
        bool FullScreen;                                        // #37
        bool Maximized;                                         // #38
        bool Minimized;                                         // #39
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
        bool NotificationsEnabled;             // #52

        int Size; // #53
    };
}