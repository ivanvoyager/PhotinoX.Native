#pragma once

#include "Photino.Strings.h"
#include "Photino.Callbacks.h"
#include "Photino.Enums.h"

#include <type_traits>

namespace PhotinoX::Native
{
    class Photino;

    struct PhotinoInitParams
    {
        static constexpr int NativeAbiVersion = 3;
        static constexpr int MaxCustomSchemeNames = 16;

        int Size;                                               // #1
        int AbiVersion;                                         // #2

        Utf8String StartString;                                 // #3
        Utf8String StartUrl;                                    // #4
        Utf8String Title;                                       // #5
        Utf8String WindowIconFile;                              // #6
        Utf8String UserDataFolder;                              // #7
        Utf8String UserAgent;                                   // #8
        Utf8String BrowserControlInitParameters;                // #9
        Utf8String Reserved;                                    // #10
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
        ContentLoadingCallback ContentLoadingHandler;           // #22
        ContentLoadedCallback ContentLoadedHandler;             // #23
        NavigationStartingCallback NavigationStartingHandler;   // #24
        NewWindowRequestedCallback NewWindowRequestedHandler;   // #25
        WebResourceRequestedCallback CustomSchemeHandler;       // #26
        ClosedCallback ClosedHandler;                           // #27
        FullScreenChangedCallback FullScreenChangedHandler;     // #28
        StateChangedCallback StateChangedHandler;               // #29
        
        int Left;                                               // #30
        int Top;                                                // #31
        int Width;                                              // #32
        int Height;                                             // #33
        int Zoom;                                               // #34
        int MinWidth;                                           // #35
        int MinHeight;                                          // #36
        int MaxWidth;                                           // #37
        int MaxHeight;                                          // #38
        
        PhotinoWindowState WindowState;                         // #39
        
        bool CenterOnInitialize;                                // #40
        bool Chromeless;                                        // #41
        bool Transparent;                                       // #42
        bool ContextMenuEnabled;                                // #43
        bool ZoomEnabled;                                       // #44
        bool DevToolsEnabled;                                   // #45
        bool Resizable;                                         // #46
        bool Topmost;                                           // #47
        bool UseOsDefaultLocation;                              // #48
        bool UseOsDefaultSize;                                  // #49
        bool GrantBrowserPermissions;                           // #50
        bool MediaAutoplayEnabled;                              // #51
        bool FileSystemAccessEnabled;                           // #52
        bool WebSecurityEnabled;                                // #53
        bool JavascriptClipboardAccessEnabled;                  // #54
        bool MediaStreamEnabled;                                // #55
        bool SmoothScrollingEnabled;                            // #56
        bool IgnoreCertificateErrorsEnabled;                    // #57
        
        bool UseNativeWindowOwner;                              // #58
        
        int ChromelessDragRegionHeight;                         // #59
        int ChromelessDragRegionLeftInset;                      // #60
        int ChromelessDragRegionRightInset;                     // #61
        int ChromelessResizeBorderThickness;                    // #62
    };

    static_assert(std::is_standard_layout_v<PhotinoInitParams>,
        "PhotinoInitParams must remain standard-layout for managed/native interop.");

    static_assert(sizeof(PhotinoInitParams) == 424,
        "PhotinoInitParams size changed. Update the managed ABI layout and size validation.");
}