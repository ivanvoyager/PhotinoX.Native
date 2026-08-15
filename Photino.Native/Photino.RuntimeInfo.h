#pragma once

namespace PhotinoX::Native
{
    struct PhotinoWindowsRuntimeInfo
    {
        const char* WebView2RuntimeVersion;
    };

    struct PhotinoLinuxRuntimeInfo
    {
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
        static constexpr int NativeAbiVersion = 1;

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
} // namespace PhotinoX::Native