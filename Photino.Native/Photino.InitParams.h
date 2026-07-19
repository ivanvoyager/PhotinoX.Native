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

        int Left;                                               // #23
        int Top;                                                // #24
        int Width;                                              // #25
        int Height;                                             // #26
        int Zoom;                                               // #27
        int MinWidth;                                           // #28
        int MinHeight;                                          // #29
        int MaxWidth;                                           // #30
        int MaxHeight;                                          // #31

        bool CenterOnInitialize;                                // #32
        bool Chromeless;                                        // #33
        bool Transparent;                                       // #34
        bool ContextMenuEnabled;                                // #35
        bool ZoomEnabled;                                       // #36
        bool DevToolsEnabled;                                   // #37
        bool FullScreen;                                        // #38
        bool Maximized;                                         // #39
        bool Minimized;                                         // #40
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

        int Size;                                               // #54
    };
}