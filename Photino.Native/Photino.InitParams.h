#pragma once

#include "Photino.Strings.h"
#include "Photino.Callbacks.h"
#include "Photino.Enums.h"

#include <cstddef>
#include <type_traits>

namespace PhotinoX::Native
{
    struct PhotinoInitCallbacks
    {
        ClosingCallback ClosingHandler;                       // #1
        ClosedCallback ClosedHandler;                         // #2
        FocusInCallback FocusInHandler;                       // #3
        FocusOutCallback FocusOutHandler;                     // #4
        ResizedCallback ResizedHandler;                       // #5
        MovedCallback MovedHandler;                           // #6
        MaximizedCallback MaximizedHandler;                   // #7
        RestoredCallback RestoredHandler;                     // #8
        MinimizedCallback MinimizedHandler;                   // #9
        FullScreenChangedCallback FullScreenChangedHandler;   // #10
        StateChangedCallback StateChangedHandler;             // #11
        WebMessageReceivedCallback WebMessageReceivedHandler; // #12
        CustomSchemeCallback CustomSchemeHandler;             // #13
        NavigationStartingCallback NavigationStartingHandler; // #14
        NewWindowRequestedCallback NewWindowRequestedHandler; // #15
        ContentLoadingCallback ContentLoadingHandler;         // #16
        ContentLoadedCallback ContentLoadedHandler;           // #17

        void* CallbackState;                                  // #18
    };
    static_assert(std::is_standard_layout_v<PhotinoInitCallbacks>,
                  "PhotinoInitCallbacks must remain standard-layout for managed/native interop.");
    static_assert(sizeof(PhotinoInitCallbacks) == 144,
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
        int DragRegionTopInset;    // #3
        int DragRegionRightInset;  // #4
        int ResizeBorderThickness; // #5
    };
    static_assert(std::is_standard_layout_v<PhotinoInitLinuxChromelessOptions>,
                  "PhotinoInitLinuxChromelessOptions must remain standard-layout for managed/native interop.");
    static_assert(sizeof(PhotinoInitLinuxChromelessOptions) == 20,
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

    class Photino;

    struct PhotinoInitParams
    {
        static constexpr int NativeAbiVersion = 7;

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

    static_assert(offsetof(PhotinoInitParams, Callbacks) == 16, "PhotinoInitParams.Callbacks offset changed.");
    static_assert(offsetof(PhotinoInitParams, Window) == 160, "PhotinoInitParams.Window offset changed.");
    static_assert(offsetof(PhotinoInitParams, LinuxChromeless) == 184,"PhotinoInitParams.LinuxChromeless offset changed.");
    static_assert(offsetof(PhotinoInitParams, Geometry) == 204, "PhotinoInitParams.Geometry offset changed.");
    static_assert(offsetof(PhotinoInitParams, Browser) == 248, "PhotinoInitParams.Browser offset changed.");

    static_assert(sizeof(PhotinoInitParams) == 432,
        "PhotinoInitParams size changed. Update the managed ABI layout and size validation.");
}