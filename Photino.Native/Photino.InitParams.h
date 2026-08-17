#pragma once

#include "Photino.Strings.h"
#include "Photino.Callbacks.h"
#include "Photino.Enums.h"

#include <type_traits>

namespace PhotinoX::Native
{
    class Photino;

    struct PhotinoInitCallbacks
    {
        ClosingCallback ClosingHandler;                       // #1
        FocusInCallback FocusInHandler;                       // #2
        FocusOutCallback FocusOutHandler;                     // #3
        ResizedCallback ResizedHandler;                       // #4
        MaximizedCallback MaximizedHandler;                   // #5
        RestoredCallback RestoredHandler;                     // #6
        MinimizedCallback MinimizedHandler;                   // #7
        MovedCallback MovedHandler;                           // #8
        WebMessageReceivedCallback WebMessageReceivedHandler; // #9
        ContentLoadingCallback ContentLoadingHandler;         // #10
        ContentLoadedCallback ContentLoadedHandler;           // #11
        NavigationStartingCallback NavigationStartingHandler; // #12
        NewWindowRequestedCallback NewWindowRequestedHandler; // #13
        WebResourceRequestedCallback CustomSchemeHandler;     // #14
        ClosedCallback ClosedHandler;                         // #15
        FullScreenChangedCallback FullScreenChangedHandler;   // #16
        StateChangedCallback StateChangedHandler;             // #17
    };
    static_assert(std::is_standard_layout_v<PhotinoInitCallbacks>,
                  "PhotinoInitCallbacks must remain standard-layout for managed/native interop.");
    static_assert(sizeof(PhotinoInitCallbacks) == 136,
                  "PhotinoInitCallbacks size changed. Update the managed ABI layout and size validation.");

    struct PhotinoInitWindowParams
    {
        Utf8String Title;           // #1
        Utf8String IconFile;        // #2

        bool Chromeless;            // #3
        bool Transparent;           // #4
        bool UseNativeWindowOwner;  // #5
    };
    static_assert(std::is_standard_layout_v<PhotinoInitWindowParams>,
                  "PhotinoInitWindowParams must remain standard-layout for managed/native interop.");
    static_assert(sizeof(PhotinoInitWindowParams) == 24,
                  "PhotinoInitWindowParams size changed. Update the managed ABI layout and size validation.");

    struct PhotinoInitLinuxChromelessOptions
    {
        int DragRegionHeight;      // #1
        int DragRegionLeftInset;   // #2
        int DragRegionRightInset;  // #3
        int ResizeBorderThickness; // #4
    };
    static_assert(std::is_standard_layout_v<PhotinoInitLinuxChromelessOptions>,
                  "PhotinoInitLinuxChromelessOptions must remain standard-layout for managed/native interop.");
    static_assert(sizeof(PhotinoInitLinuxChromelessOptions) == 16,
                  "PhotinoInitLinuxChromelessOptions size changed. Update the managed ABI layout and size validation.");

    struct PhotinoInitGeometry
    {
        int Left;                       // #1
        int Top;                        // #2
        int Width;                      // #3
        int Height;                     // #4
        int MinWidth;                   // #5
        int MinHeight;                  // #6
        int MaxWidth;                   // #7
        int MaxHeight;                  // #8

        PhotinoWindowState WindowState; // #9

        bool CenterOnInitialize;        // #10
        bool Resizable;                 // #11
        bool Topmost;                   // #12
        bool UseOsDefaultLocation;      // #13
        bool UseOsDefaultSize;          // #14
    };
    static_assert(std::is_standard_layout_v<PhotinoInitGeometry>,
                  "PhotinoInitGeometry must remain standard-layout for managed/native interop.");
    static_assert(sizeof(PhotinoInitGeometry) == 44,
                  "PhotinoInitGeometry size changed. Update the managed ABI layout and size validation.");

    inline constexpr int PhotinoMaxCustomSchemeNames = 16;

    struct PhotinoInitBrowserParams
    {
        Utf8String StartString;                                     // #1
        Utf8String StartUrl;                                        // #2

        Utf8String UserDataFolder;                                  // #3
        Utf8String UserAgent;                                       // #4
        Utf8String ControlInitParameters;                           // #5
        Utf8String CustomSchemeNames[PhotinoMaxCustomSchemeNames];  // #6

        int Zoom;                                                   // #7
        bool ZoomEnabled;                                           // #8
        bool ContextMenuEnabled;                                    // #9
        bool StatusBarEnabled;                                      // #10
        bool DevToolsEnabled;                                       // #11

        bool GrantBrowserPermissions;                               // #12
        bool MediaAutoplayEnabled;                                  // #13
        bool FileSystemAccessEnabled;                               // #14
        bool WebSecurityEnabled;                                    // #15
        bool JavascriptClipboardAccessEnabled;                      // #16
        bool MediaStreamEnabled;                                    // #17
        bool SmoothScrollingEnabled;                                // #18
        bool IgnoreCertificateErrorsEnabled;                        // #19
    };
    static_assert(std::is_standard_layout_v<PhotinoInitBrowserParams>,
                  "PhotinoInitBrowserParams must remain standard-layout for managed/native interop.");
    static_assert(sizeof(PhotinoInitBrowserParams) == 184,
                  "PhotinoInitBrowserParams size changed. Update the managed ABI layout and size validation.");

    struct PhotinoInitParams
    {
        static constexpr int NativeAbiVersion = 5;

        int Size;                                           // #1
        int AbiVersion;                                     // #2

        Photino* ParentInstance;                            // #3

        PhotinoInitCallbacks Callbacks;                     // #4
        PhotinoInitWindowParams Window;                     // #5
        PhotinoInitLinuxChromelessOptions LinuxChromeless;  // #6
        PhotinoInitGeometry Geometry;                       // #7
        PhotinoInitBrowserParams Browser;                   // #8
    };
    static_assert(std::is_standard_layout_v<PhotinoInitParams>,
        "PhotinoInitParams must remain standard-layout for managed/native interop.");
    static_assert(sizeof(PhotinoInitParams) == 424,
        "PhotinoInitParams size changed. Update the managed ABI layout and size validation.");
}