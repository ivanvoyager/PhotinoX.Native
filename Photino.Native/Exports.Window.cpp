#include "Photino.Export.h"
#include "Photino.h"
#include "Photino.Strings.h"

#include <cassert>

using namespace PhotinoX::Native;

extern "C"
{
#ifdef _WIN32

    PHOTINO_EXPORT HWND Photino_getHwnd_win32(const Photino* instance)
    {
        assert(instance);
        if (!instance) return nullptr;

        return instance->GetHwnd();
    }

#elif defined(__linux__)

    PHOTINO_EXPORT void* Photino_getGtkWidget_linux(const Photino* instance)
    {
        assert(instance);
        if (!instance) return nullptr;

        return instance->GetGtkWidget();
    }

#elif defined(__APPLE__)

    PHOTINO_EXPORT void* Photino_getNSWindow_mac(const Photino* instance)
    {
        assert(instance);
        if (!instance) return nullptr;

        return instance->GetNSWindow();
    }

#endif

    PHOTINO_EXPORT bool Photino_Show(const Photino* instance)
    {
        assert(instance);
        if (!instance) return false;

        return instance->Show();
    }

    PHOTINO_EXPORT bool Photino_Activate(const Photino* instance)
    {
        assert(instance);
        if (!instance) return false;

        return instance->Activate();
    }

    PHOTINO_EXPORT bool Photino_Center(const Photino* instance)
    {
        assert(instance);
        if (!instance) return false;

        return instance->Center();
    }

    PHOTINO_EXPORT bool Photino_Maximize(Photino* instance)
    {
        assert(instance);
        if (!instance) return false;

        return instance->Maximize();
    }

    PHOTINO_EXPORT bool Photino_Minimize(Photino* instance)
    {
        assert(instance);
        if (!instance) return false;

        return instance->Minimize();
    }

    PHOTINO_EXPORT bool Photino_Restore(Photino* instance)
    {
        assert(instance);
        if (!instance) return false;

        return instance->Restore();
    }

    PHOTINO_EXPORT void Photino_Close(const Photino* instance)
    {
        assert(instance);
        if (!instance) return;

        instance->Close();
    }


    PHOTINO_EXPORT char* Photino_GetTitle(const Photino* instance)
    {
        assert(instance);
        if (!instance) return nullptr;

        std::string value = ToUtf8String(instance->GetTitle());
        return CopyUtf8String(value);
    }

    PHOTINO_EXPORT void Photino_SetTitle(Photino* instance, Utf8String title)
    {
        assert(instance);
        if (!instance) return;

        instance->SetTitle(ToPlatformString(title));
    }


    PHOTINO_EXPORT char* Photino_GetIconFile(const Photino* instance)
    {
        assert(instance);
        if (!instance) return nullptr;

        std::string value = ToUtf8String(instance->GetIconFile());
        return CopyUtf8String(value);
    }

    PHOTINO_EXPORT void Photino_SetIconFile(Photino* instance, Utf8String filename)
    {
        assert(instance);
        if (!instance || !filename) return;

        instance->SetIconFile(ToPlatformString(filename));
    }


    PHOTINO_EXPORT void Photino_GetPosition(const Photino* instance, int* x, int* y)
    {
        assert(instance);
        if (!instance || (!x && !y)) return;

        instance->GetPosition(x, y);
    }

    PHOTINO_EXPORT void Photino_SetPosition(Photino* instance, const int x, const int y)
    {
        assert(instance);
        if (!instance) return;

        instance->SetPosition(x, y);
    }


    PHOTINO_EXPORT void Photino_GetSize(const Photino* instance, int* width, int* height)
    {
        assert(instance);
        if (!instance || (!width && !height)) return;

        instance->GetSize(width, height);
    }

    PHOTINO_EXPORT void Photino_SetSize(Photino* instance, const int width, const int height)
    {
        assert(instance);
        if (!instance) return;

        instance->SetSize(width, height);
    }


    PHOTINO_EXPORT void Photino_SetMinSize(Photino* instance, const int width, const int height)
    {
        assert(instance);
        if (!instance) return;

        instance->SetMinSize(width, height);
    }

    PHOTINO_EXPORT void Photino_SetMaxSize(Photino* instance, const int width, const int height)
    {
        assert(instance);
        if (!instance) return;

        instance->SetMaxSize(width, height);
    }


    PHOTINO_EXPORT void Photino_BeginWindowDrag(const Photino* instance)
    {
        assert(instance);
        if (!instance) return;

        instance->BeginWindowDrag();
    }

    PHOTINO_EXPORT void Photino_BeginWindowResize(const Photino* instance, const WindowEdge edge)
    {
        assert(instance);
        if (!instance) return;

        instance->BeginWindowResize(edge);
    }


    PHOTINO_EXPORT unsigned int Photino_GetScreenDpi(const Photino* instance)
    {
        assert(instance);
        if (!instance) return 0;

        return instance->GetScreenDpi();
    }

    PHOTINO_EXPORT void Photino_GetAllMonitors(const Photino* instance, const GetAllMonitorsCallback callback)
    {
        assert(instance);
        if (!instance || !callback) return;
        instance->GetAllMonitors(callback);
    }


    PHOTINO_EXPORT void Photino_GetFullScreen(const Photino* instance, bool* fullScreen)
    {
        assert(instance);
        if (!instance || !fullScreen) return;

        instance->GetFullScreen(fullScreen);
    }

    PHOTINO_EXPORT void Photino_SetFullScreen(Photino* instance, const bool fullScreen)
    {
        assert(instance);
        if (!instance) return;

        instance->SetFullScreen(fullScreen);
    }


    PHOTINO_EXPORT void Photino_GetMaximized(const Photino* instance, bool* isMaximized)
    {
        assert(instance);
        if (!instance || !isMaximized) return;

        instance->GetMaximized(isMaximized);
    }

    PHOTINO_EXPORT void Photino_SetMaximized(Photino* instance, const bool maximized)
    {
        assert(instance);
        if (!instance) return;

        instance->SetMaximized(maximized);
    }


    PHOTINO_EXPORT void Photino_GetMinimized(const Photino* instance, bool* isMinimized)
    {
        assert(instance);
        if (!instance || !isMinimized) return;

        instance->GetMinimized(isMinimized);
    }

    PHOTINO_EXPORT void Photino_SetMinimized(Photino* instance, const bool minimized)
    {
        assert(instance);
        if (!instance) return;

        instance->SetMinimized(minimized);
    }


    PHOTINO_EXPORT void Photino_GetWindowState(const Photino* instance, WindowState* state)
    {
        assert(instance);
        assert(state);
        if (!instance || !state) return;

        instance->GetWindowState(state);
    }

    PHOTINO_EXPORT void Photino_SetWindowState(Photino* instance, const WindowState state)
    {
        assert(instance);
        if (!instance) return;

        instance->SetWindowState(state);
    }


    PHOTINO_EXPORT void Photino_GetResizable(const Photino* instance, bool* resizable)
    {
        assert(instance);
        if (!instance || !resizable) return;

        instance->GetResizable(resizable);
    }

    PHOTINO_EXPORT void Photino_SetResizable(Photino* instance, const bool resizable)
    {
        assert(instance);
        if (!instance) return;

        instance->SetResizable(resizable);
    }


    PHOTINO_EXPORT void Photino_GetTopmost(const Photino* instance, bool* topmost)
    {
        assert(instance);
        if (!instance || !topmost) return;

        instance->GetTopmost(topmost);
    }

    PHOTINO_EXPORT void Photino_SetTopmost(Photino* instance, const bool topmost)
    {
        assert(instance);
        if (!instance) return;
        instance->SetTopmost(topmost);
    }
}