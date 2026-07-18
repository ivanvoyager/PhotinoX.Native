#include "Photino.Export.h"
#include "Photino.h"
#include "Photino.Strings.h"

#include <cassert>

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

    PHOTINO_EXPORT void Photino_GetNotificationsEnabled(const Photino* instance, bool* enabled)
    {
        assert(instance);
        if (!instance || !enabled) return;

        instance->GetNotificationsEnabled(enabled);
    }

    PHOTINO_EXPORT void Photino_ShowNotification(const Photino* instance, Utf8String title, Utf8String body)
    {
        assert(instance);
        if (!instance) return;

        instance->ShowNotification(ToPlatformString(title), ToPlatformString(body));

    }
}
