#include "Photino.h"
#include "Photino.Callbacks.h"
#include "Photino.InitParams.h"
#include "Photino.Strings.h"
#include "Photino.Export.h"

#include <cassert>

using namespace PhotinoX::Native;

extern "C"
{
#ifdef _WIN32
    PHOTINO_EXPORT void Photino_register_win32(const HINSTANCE hInstance)
    {
        Photino::Register(hInstance);
    }

    PHOTINO_EXPORT void Photino_setWebView2RuntimePath_win32(const wchar_t* webView2RuntimePath)
    {
        Photino::SetWebView2RuntimePath(webView2RuntimePath ? PlatformString(webView2RuntimePath) : PlatformString());
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

    PHOTINO_EXPORT void Photino_GetNotificationsEnabled(const Photino* instance, bool* disabled)
    {
        assert(instance);
        if (!instance || !disabled) return;

        instance->GetNotificationsEnabled(disabled);
    }

    PHOTINO_EXPORT void Photino_ShowNotification(const Photino* instance, Utf8String title, Utf8String body)
    {
        assert(instance);
        if (!instance) return;

        instance->ShowNotification(ToPlatformString(title), ToPlatformString(body));

    }

    PHOTINO_EXPORT void Photino_WaitForExit(const Photino* instance)
    {
        assert(instance);
        if (!instance) return;

        instance->WaitForExit();
    }

    PHOTINO_EXPORT void Photino_Invoke(const Photino* instance, const InvokeCallback callback)
    {
        assert(instance);
        if (!instance || !callback) return;

        instance->Invoke(callback);
    }
}
