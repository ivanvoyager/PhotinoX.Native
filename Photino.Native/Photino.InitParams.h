#pragma once

#include "Photino.Strings.h"
#include "Photino.Callbacks.h"
#include "Photino.Enums.h"

#include <type_traits>

namespace PhotinoX::Native
{
    class Photino;

    inline constexpr int PhotinoNativeAbiVersion = 1;
    
    inline constexpr int MaxCustomSchemeNames = 16;

    struct PhotinoInitParams
    {
        int Size;                                               // #1
        int AbiVersion;                                         // #2

        Utf8String StartString;                                 // #3
        Utf8String StartUrl;                                    // #4
        Utf8String Title;                                       // #5
        Utf8String WindowIconFile;                              // #6
        Utf8String UserDataFolder;                              // #7
        Utf8String UserAgent;                                   // #8
        Utf8String BrowserControlInitParameters;                // #9
        Utf8String NotificationRegistrationId;                  // #10
        Utf8String CustomSchemeNames[MaxCustomSchemeNames];     // #11
        
        Photino* ParentInstance;                                // #12
        
        ClosingCallback ClosingHandler;                         // #13
        FocusInCallback FocusInHandler;                         // #14
        FocusOutCallback FocusOutHandler;                       // #15
        ResizedCallback ResizedHandler;                         // #16
        MaximizedCallback MaximizedHandler;                     // #17
        RestoredCallback RestoredHandler;                       // #18
        MinimizedCallback MinimizedHandler;                     // #19
        MovedCallback MovedHandler;                             // #20
        WebMessageReceivedCallback WebMessageReceivedHandler;   // #21
        ContentLoadedCallback ContentLoadedHandler;             // #22
        NavigationStartingCallback NavigationStartingHandler;   // #23
        NewWindowRequestedCallback NewWindowRequestedHandler;   // #24
        WebResourceRequestedCallback CustomSchemeHandler;       // #25
        ClosedCallback ClosedHandler;                           // #26
        FullScreenChangedCallback FullScreenChangedHandler;     // #27
        StateChangedCallback StateChangedHandler;               // #28
        
        int Left;                                               // #29
        int Top;                                                // #30
        int Width;                                              // #31
        int Height;                                             // #32
        int Zoom;                                               // #33
        int MinWidth;                                           // #34
        int MinHeight;                                          // #35
        int MaxWidth;                                           // #36
        int MaxHeight;                                          // #37
        
        PhotinoWindowState WindowState;                         // #38
        
        bool CenterOnInitialize;                                // #39
        bool Chromeless;                                        // #40
        bool Transparent;                                       // #41
        bool ContextMenuEnabled;                                // #42
        bool ZoomEnabled;                                       // #43
        bool DevToolsEnabled;                                   // #44
        bool Resizable;                                         // #45
        bool Topmost;                                           // #46
        bool UseOsDefaultLocation;                              // #47
        bool UseOsDefaultSize;                                  // #48
        bool GrantBrowserPermissions;                           // #49
        bool MediaAutoplayEnabled;                              // #50
        bool FileSystemAccessEnabled;                           // #51
        bool WebSecurityEnabled;                                // #52
        bool JavascriptClipboardAccessEnabled;                  // #53
        bool MediaStreamEnabled;                                // #54
        bool SmoothScrollingEnabled;                            // #55
        bool IgnoreCertificateErrorsEnabled;                    // #56
        bool NotificationsEnabled;                              // #57
        
        bool UseNativeWindowOwner;                              // #58
        
        int ChromelessDragRegionHeight;                         // #59
        int ChromelessDragRegionLeftInset;                      // #60
        int ChromelessDragRegionRightInset;                     // #61
        int ChromelessResizeBorderThickness;                    // #62
    };

    static_assert(std::is_standard_layout_v<PhotinoInitParams>,
        "PhotinoInitParams must remain standard-layout for managed/native interop.");

    static_assert(sizeof(PhotinoWindowState) == sizeof(int),
        "PhotinoWindowState must remain int-sized for managed/native interop.");

    static_assert(sizeof(PhotinoInitParams) == 416,
        "PhotinoInitParams size changed. Update the managed ABI layout and size validation.");
}