#include "Photino.Export.h"
#include "Photino.Strings.h"
#include "Photino.h"

#include <cassert>

using namespace PhotinoX::Native;

extern "C"
{
    PHOTINO_EXPORT void Photino_GetTransparentEnabled(const Photino* instance, bool* enabled)
    {
        assert(instance);
        if (!instance || !enabled) return;

        instance->GetTransparentEnabled(enabled);
    }

    PHOTINO_EXPORT void Photino_SetTransparentEnabled(Photino* instance, const bool enabled)
    {
        assert(instance);
        if (!instance) return;

        instance->SetTransparentEnabled(enabled);
    }


    PHOTINO_EXPORT void Photino_ClearBrowserAutoFill(const Photino* instance)
    {
        assert(instance);
        if (!instance) return;

        instance->ClearBrowserAutoFill();
    }


    PHOTINO_EXPORT void Photino_NavigateToString(const Photino* instance, Utf8String content)
    {
        assert(instance);
        if (!instance || !content) return;

        instance->NavigateToString(ToPlatformString(content));
    }

    PHOTINO_EXPORT void Photino_NavigateToUrl(const Photino* instance, Utf8String url)
    {
        assert(instance);
        if (!instance || !url) return;

        instance->NavigateToUrl(ToPlatformString(url));
    }


    PHOTINO_EXPORT void Photino_SendWebMessage(const Photino* instance, Utf8String message)
    {
        assert(instance);
        if (!instance || !message) return;

        instance->SendWebMessage(ToPlatformString(message));
    }


    PHOTINO_EXPORT bool Photino_AddCustomSchemeName(Photino* instance, Utf8String scheme)
    {
        assert(instance);
        if (!instance || !scheme) return false;
        return instance->AddCustomSchemeName(scheme);
    }


    PHOTINO_EXPORT void Photino_GetContextMenuEnabled(const Photino* instance, bool* enabled)
    {
        assert(instance);
        if (!instance || !enabled) return;

        instance->GetContextMenuEnabled(enabled);
    }

    PHOTINO_EXPORT void Photino_SetContextMenuEnabled(Photino* instance, const bool enabled)
    {
        assert(instance);
        if (!instance) return;

        instance->SetContextMenuEnabled(enabled);
    }


    PHOTINO_EXPORT void Photino_GetZoomEnabled(const Photino* instance, bool* enabled)
    {
        assert(instance);
        if (!instance || !enabled) return;

        instance->GetZoomEnabled(enabled);
    }

    PHOTINO_EXPORT void Photino_SetZoomEnabled(Photino* instance, const bool enabled)
    {
        assert(instance);
        if (!instance) return;

        instance->SetZoomEnabled(enabled);
    }

    PHOTINO_EXPORT void Photino_GetStatusBarEnabled(const Photino* instance, bool* enabled)
    {
        assert(instance);
        if (!instance || !enabled) return;

        instance->GetStatusBarEnabled(enabled);
    }

    PHOTINO_EXPORT void Photino_SetStatusBarEnabled(Photino* instance, const bool enabled)
    {
        assert(instance);
        if (!instance) return;

        instance->SetStatusBarEnabled(enabled);
    }


    PHOTINO_EXPORT void Photino_GetZoom(const Photino* instance, int* zoom)
    {
        assert(instance);
        if (!instance || !zoom) return;

        instance->GetZoom(zoom);
    }

    PHOTINO_EXPORT void Photino_SetZoom(Photino* instance, const int zoom)
    {
        assert(instance);
        if (!instance) return;
        instance->SetZoom(zoom);
    }


    PHOTINO_EXPORT void Photino_GetDevToolsEnabled(const Photino* instance, bool* enabled)
    {
        assert(instance);
        if (!instance || !enabled) return;

        instance->GetDevToolsEnabled(enabled);
    }

    PHOTINO_EXPORT void Photino_SetDevToolsEnabled(Photino* instance, const bool enabled)
    {
        assert(instance);
        if (!instance) return;

        instance->SetDevToolsEnabled(enabled);
    }


    PHOTINO_EXPORT void Photino_GetGrantBrowserPermissions(const Photino* instance, bool* grant)
    {
        assert(instance);
        if (!instance || !grant) return;

        instance->GetGrantBrowserPermissions(grant);
    }

    PHOTINO_EXPORT char* Photino_GetUserAgent(const Photino* instance)
    {
        assert(instance);
        if (!instance) return nullptr;

        std::string value = ToUtf8String(instance->GetUserAgent());
        return CopyUtf8String(value);
    }

    PHOTINO_EXPORT void Photino_GetMediaAutoplayEnabled(const Photino* instance, bool* enabled)
    {
        assert(instance);
        if (!instance || !enabled) return;

        instance->GetMediaAutoplayEnabled(enabled);
    }

    PHOTINO_EXPORT void Photino_GetFileSystemAccessEnabled(const Photino* instance, bool* enabled)
    {
        assert(instance);
        if (!instance || !enabled) return;

        instance->GetFileSystemAccessEnabled(enabled);
    }

    PHOTINO_EXPORT void Photino_GetWebSecurityEnabled(const Photino* instance, bool* enabled)
    {
        assert(instance);
        if (!instance || !enabled) return;

        instance->GetWebSecurityEnabled(enabled);
    }

    PHOTINO_EXPORT void Photino_GetJavascriptClipboardAccessEnabled(const Photino* instance, bool* enabled)
    {
        assert(instance);
        if (!instance || !enabled) return;

        instance->GetJavascriptClipboardAccessEnabled(enabled);
    }

    PHOTINO_EXPORT void Photino_GetMediaStreamEnabled(const Photino* instance, bool* enabled)
    {
        assert(instance);
        if (!instance || !enabled) return;

        instance->GetMediaStreamEnabled(enabled);
    }

    PHOTINO_EXPORT void Photino_GetSmoothScrollingEnabled(const Photino* instance, bool* enabled)
    {
        assert(instance);
        if (!instance || !enabled) return;

        instance->GetSmoothScrollingEnabled(enabled);
    }

    PHOTINO_EXPORT void Photino_GetIgnoreCertificateErrorsEnabled(const Photino* instance, bool* disabled)
    {
        assert(instance);
        if (!instance || !disabled) return;

        instance->GetIgnoreCertificateErrorsEnabled(disabled);
    }

#ifdef _WIN32

    PHOTINO_EXPORT void Photino_setWebView2RuntimePath_win32(const wchar_t* webView2RuntimePath)
    {
        Photino::SetWebView2RuntimePath(webView2RuntimePath ? PlatformString(webView2RuntimePath) : PlatformString());
    }

#endif
}