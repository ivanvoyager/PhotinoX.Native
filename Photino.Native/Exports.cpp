#include "Photino.Export.h"
#include "Photino.RuntimeInfo.h"
#include "Photino.h"
#include "version.h"

using namespace PhotinoX::Native;

extern "C"
{
#ifdef _WIN32

    PHOTINO_EXPORT void Photino_register_win32(const HINSTANCE hInstance)
    {
        Photino::Register(hInstance);
    }

#elif defined(__linux__)

    PHOTINO_EXPORT void Photino_register_linux()
    {
        Photino::Register();
    }

#elif defined(__APPLE__)

    PHOTINO_EXPORT void Photino_register_mac()
    {
        Photino::Register();
    }

#endif

    PHOTINO_EXPORT Photino* Photino_ctor(PhotinoInitParams* initParams)
    {
        return new Photino(initParams);
    }

    PHOTINO_EXPORT const char* Photino_GetNativeVersion()
    {
        return VER_STR;
    }

    PHOTINO_EXPORT PhotinoRuntimeInfo Photino_GetRuntimeInfo()
    {
        PhotinoRuntimeInfo info{};
        info.Size = sizeof(PhotinoRuntimeInfo);
        info.AbiVersion = PhotinoRuntimeInfo::NativeAbiVersion;

        info.NativeVersion = VER_STR;

#ifdef _WIN32
        info.WebViewEngine = "WebView2";
        info.WebViewRuntimeVersion = info.Windows.WebView2RuntimeVersion = Photino::GetWebView2RuntimeVersion();
#elif defined(__linux__)
        info.WebViewEngine = "WebKitGTK";
        info.Linux.GlibcVersion = Photino::GetGlibcVersion();
        info.Linux.GtkVersion = Photino::GetGtkVersion();
        info.Linux.WebKitGtkApiTarget = "WebKitGTK 4.1";
        info.WebViewRuntimeVersion = info.Linux.WebKitGtkRuntimeVersion = Photino::GetWebKitGtkRuntimeVersion();
#elif defined(__APPLE__)
        info.WebViewEngine = "WKWebView";
        info.WebViewRuntimeVersion = info.MacOS.WebKitVersion = Photino::GetWebKitVersion();
#endif

        return info;
    }
}