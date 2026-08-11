#include "Photino.Export.h"
#include "Photino.h"

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
}