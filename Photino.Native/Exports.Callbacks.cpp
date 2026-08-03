#include "Photino.Export.h"
#include "Photino.Callbacks.h"
#include "Photino.h"

#include <cassert>

using namespace PhotinoX::Native;

extern "C"
{
    PHOTINO_EXPORT void Photino_SetClosingCallback(Photino* instance, const ClosingCallback callback)
    {
        assert(instance);
        if (!instance) return;
        instance->SetClosingCallback(callback);
    }

    PHOTINO_EXPORT void Photino_SetClosedCallback(Photino* instance, const ClosedCallback callback)
    {
        assert(instance);
        if (!instance) return;
        instance->SetClosedCallback(callback);
    }

    PHOTINO_EXPORT void Photino_SetFocusInCallback(Photino* instance, const FocusInCallback callback)
    {
        assert(instance);
        if (!instance) return;
        instance->SetFocusInCallback(callback);
    }

    PHOTINO_EXPORT void Photino_SetFocusOutCallback(Photino* instance, const FocusOutCallback callback)
    {
        assert(instance);
        if (!instance) return;
        instance->SetFocusOutCallback(callback);
    }

    PHOTINO_EXPORT void Photino_SetMovedCallback(Photino* instance, const MovedCallback callback)
    {
        assert(instance);
        if (!instance) return;
        instance->SetMovedCallback(callback);
    }

    PHOTINO_EXPORT void Photino_SetResizedCallback(Photino* instance, const ResizedCallback callback)
    {
        assert(instance);
        if (!instance) return;
        instance->SetResizedCallback(callback);
    }

    PHOTINO_EXPORT void Photino_SetMaximizedCallback(Photino* instance, const MaximizedCallback callback)
    {
        assert(instance);
        if (!instance) return;
        instance->SetMaximizedCallback(callback);
    }

    PHOTINO_EXPORT void Photino_SetRestoredCallback(Photino* instance, const RestoredCallback callback)
    {
        assert(instance);
        if (!instance) return;
        instance->SetRestoredCallback(callback);
    }

    PHOTINO_EXPORT void Photino_SetMinimizedCallback(Photino* instance, const MinimizedCallback callback)
    {
        assert(instance);
        if (!instance) return;
        instance->SetMinimizedCallback(callback);
    }

    PHOTINO_EXPORT void Photino_SetFullScreenChangedCallback(Photino* instance, const FullScreenChangedCallback callback)
    {
        assert(instance);
        if (!instance) return;
        instance->SetFullScreenChangedCallback(callback);
    }

    PHOTINO_EXPORT void Photino_SetStateChangedCallback(Photino* instance, const StateChangedCallback callback)
    {
        assert(instance);
        if (!instance) return;
        instance->SetStateChangedCallback(callback);
    }

    PHOTINO_EXPORT void Photino_SetNavigationStartingCallback(Photino* instance, const NavigationStartingCallback callback)
    {
        assert(instance);
        if (!instance) return;
        instance->SetNavigationStartingCallback(callback);
    }

    PHOTINO_EXPORT void Photino_SetNewWindowRequestedCallback(Photino* instance, const NewWindowRequestedCallback callback)
    {
        assert(instance);
        if (!instance) return;
        instance->SetNewWindowRequestedCallback(callback);
    }

    PHOTINO_EXPORT void Photino_SetContentLoadedCallback(Photino* instance, const ContentLoadedCallback callback)
    {
        assert(instance);
        if (!instance) return;
        instance->SetContentLoadedCallback(callback);
    }
}