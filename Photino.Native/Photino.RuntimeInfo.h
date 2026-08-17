#pragma once

#include <type_traits>

namespace PhotinoX::Native
{
    struct PhotinoWindowsRuntimeInfo
    {
        const char* WebView2RuntimeVersion;
    };

    struct PhotinoLinuxRuntimeInfo
    {
        const char* GlibcVersion;
        const char* GtkVersion;
        const char* WebKitGtkApiTarget;
        const char* WebKitGtkRuntimeVersion;
    };

    struct PhotinoMacOSRuntimeInfo
    {
        const char* WebKitVersion;
    };

    struct PhotinoRuntimeInfo
    {
        static constexpr int NativeAbiVersion = 2;

        int Size;                               // #1
        int AbiVersion;                         // #2

        const char* NativeVersion;              // #3

        const char* WebViewEngine;              // #4
        const char* WebViewRuntimeVersion;      // #5

        union
        {
            PhotinoWindowsRuntimeInfo Windows;  // #6
            PhotinoLinuxRuntimeInfo Linux;      // #6
            PhotinoMacOSRuntimeInfo MacOS;      // #6
        };
    };

    static_assert(std::is_standard_layout_v<PhotinoRuntimeInfo>,
                  "PhotinoRuntimeInfo must remain standard-layout for managed/native interop.");

    static_assert(sizeof(PhotinoRuntimeInfo) == 64,
                  "PhotinoRuntimeInfo size changed. Update the managed ABI layout and size validation.");
    } // namespace PhotinoX::Native