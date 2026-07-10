#include "Photino.h"
#include "Photino.Dialog.h"
#include "Photino.InitParams.h"
#include "Photino.Memory.h"
#include "Photino.Strings.h"

#include <cassert>
#include <new>

#ifdef _WIN32
#define EXPORTED __declspec(dllexport)
#else
#define EXPORTED
#endif

using namespace PhotinoX::Native;

extern "C"
{
#ifdef _WIN32
    EXPORTED void Photino_register_win32(const HINSTANCE hInstance)
    {
        Photino::Register(hInstance);
    }

    EXPORTED HWND Photino_getHwnd_win32(const Photino* instance)
    {
        assert(instance);
        if (!instance) return nullptr;

        return instance->GetHwnd();
    }

    EXPORTED void Photino_setWebView2RuntimePath_win32(const wchar_t* webView2RuntimePath)
    {
        Photino::SetWebView2RuntimePath(webView2RuntimePath ? PlatformString(webView2RuntimePath) : PlatformString());
    }
#elif __linux__
    EXPORTED void Photino_register_linux()
    {
        Photino::Register();
    }
#elif __APPLE__
    EXPORTED void Photino_register_mac()
    {
        Photino::Register();
    }
#endif

    EXPORTED Photino* Photino_ctor(PhotinoInitParams* initParams)
    {
        return new Photino(initParams);
    }

    EXPORTED void Photino_Center(Photino* instance)
    {
        assert(instance);
        if (!instance) return;

        instance->Center();
    }

    EXPORTED void Photino_ClearBrowserAutoFill(const Photino* instance)
    {
        assert(instance);
        if (!instance) return;

        instance->ClearBrowserAutoFill();
    }

    EXPORTED void Photino_Close(const Photino* instance)
    {
        assert(instance);
        if (!instance) return;

        instance->Close();
    }

    EXPORTED void Photino_GetNotificationsEnabled(const Photino* instance, bool* disabled)
    {
        assert(instance);
        if (!instance || !disabled) return;

        instance->GetNotificationsEnabled(disabled);
    }

    EXPORTED void Photino_GetTransparentEnabled(const Photino* instance, bool* enabled)
    {
        assert(instance);
        if (!instance || !enabled) return;

        instance->GetTransparentEnabled(enabled);
    }

    EXPORTED void Photino_GetContextMenuEnabled(const Photino* instance, bool* enabled)
    {
        assert(instance);
        if (!instance || !enabled) return;

        instance->GetContextMenuEnabled(enabled);
    }

    EXPORTED void Photino_GetZoomEnabled(const Photino* instance, bool* enabled)
    {
        assert(instance);
        if (!instance || !enabled) return;

        instance->GetZoomEnabled(enabled);
    }

    EXPORTED void Photino_GetDevToolsEnabled(const Photino* instance, bool* enabled)
    {
        assert(instance);
        if (!instance || !enabled) return;

        instance->GetDevToolsEnabled(enabled);
    }

    EXPORTED void Photino_GetFullScreen(const Photino* instance, bool* fullScreen)
    {
        assert(instance);
        if (!instance || !fullScreen) return;

        instance->GetFullScreen(fullScreen);
    }

    EXPORTED void Photino_GetGrantBrowserPermissions(const Photino* instance, bool* grant)
    {
        assert(instance);
        if (!instance || !grant) return;

        instance->GetGrantBrowserPermissions(grant);
    }

    EXPORTED char* Photino_GetIconFileName(const Photino* instance)
    {
        assert(instance);
        if (!instance) return nullptr;

        std::string value = ToUtf8String(instance->GetIconFileName());
        return CopyUtf8String(value);
    }

    EXPORTED char* Photino_GetUserAgent(const Photino* instance)
    {
        assert(instance);
        if (!instance) return nullptr;

        std::string value = ToUtf8String(instance->GetUserAgent());
        return CopyUtf8String(value);
    }

    EXPORTED void Photino_GetMediaAutoplayEnabled(const Photino* instance, bool* enabled)
    {
        assert(instance);
        if (!instance || !enabled) return;

        instance->GetMediaAutoplayEnabled(enabled);
    }

    EXPORTED void Photino_GetFileSystemAccessEnabled(const Photino* instance, bool* enabled)
    {
        assert(instance);
        if (!instance || !enabled) return;

        instance->GetFileSystemAccessEnabled(enabled);
    }

    EXPORTED void Photino_GetWebSecurityEnabled(const Photino* instance, bool* enabled)
    {
        assert(instance);
        if (!instance || !enabled) return;

        instance->GetWebSecurityEnabled(enabled);
    }

    EXPORTED void Photino_GetJavascriptClipboardAccessEnabled(const Photino* instance, bool* enabled)
    {
        assert(instance);
        if (!instance || !enabled) return;

        instance->GetJavascriptClipboardAccessEnabled(enabled);
    }

    EXPORTED void Photino_GetMediaStreamEnabled(const Photino* instance, bool* enabled)
    {
        assert(instance);
        if (!instance || !enabled) return;

        instance->GetMediaStreamEnabled(enabled);
    }

    EXPORTED void Photino_GetSmoothScrollingEnabled(const Photino* instance, bool* enabled)
    {
        assert(instance);
        if (!instance || !enabled) return;

        instance->GetSmoothScrollingEnabled(enabled);
    }

    EXPORTED void Photino_GetMaximized(const Photino* instance, bool* isMaximized)
    {
        assert(instance);
        if (!instance || !isMaximized) return;

        instance->GetMaximized(isMaximized);
    }

    EXPORTED void Photino_GetMinimized(const Photino* instance, bool* isMinimized)
    {
        assert(instance);
        if (!instance || !isMinimized) return;

        instance->GetMinimized(isMinimized);
    }

    EXPORTED void Photino_GetIgnoreCertificateErrorsEnabled(const Photino* instance, bool* disabled)
    {
        assert(instance);
        if (!instance || !disabled) return;

        instance->GetIgnoreCertificateErrorsEnabled(disabled);
    }

    EXPORTED void Photino_GetPosition(const Photino* instance, int* x, int* y)
    {
        assert(instance);
        if (!instance || (!x && !y)) return;

        instance->GetPosition(x, y);
    }

    EXPORTED void Photino_GetResizable(const Photino* instance, bool* resizable)
    {
        assert(instance);
        if (!instance || !resizable) return;

        instance->GetResizable(resizable);
    }

    EXPORTED unsigned int Photino_GetScreenDpi(const Photino* instance)
    {
        assert(instance);
        if (!instance) return 0;

        return instance->GetScreenDpi();
    }

    EXPORTED void Photino_GetSize(const Photino* instance, int* width, int* height)
    {
        assert(instance);
        if (!instance || (!width && !height)) return;

        instance->GetSize(width, height);
    }

    EXPORTED char* Photino_GetTitle(const Photino* instance)
    {
        assert(instance);
        if (!instance) return nullptr;

        std::string value = ToUtf8String(instance->GetTitle());
        return CopyUtf8String(value);
    }

    EXPORTED void Photino_GetTopmost(const Photino* instance, bool* topmost)
    {
        assert(instance);
        if (!instance || !topmost) return;

        instance->GetTopmost(topmost);
    }

    EXPORTED void Photino_GetZoom(const Photino* instance, int* zoom)
    {
        assert(instance);
        if (!instance || !zoom) return;

        instance->GetZoom(zoom);
    }

    EXPORTED void Photino_NavigateToString(const Photino* instance, Utf8String content)
    {
        assert(instance);
        if (!instance || !content) return;

        instance->NavigateToString(ToPlatformString(content));
    }

    EXPORTED void Photino_NavigateToUrl(const Photino* instance, Utf8String url)
    {
        assert(instance);
        if (!instance || !url) return;

        instance->NavigateToUrl(ToPlatformString(url));
    }

    EXPORTED void Photino_Restore(const Photino* instance)
    {
        assert(instance);
        if (!instance) return;

        instance->Restore();
    }

    EXPORTED void Photino_SendWebMessage(const Photino* instance, Utf8String message)
    {
        assert(instance);
        if (!instance || !message) return;

        instance->SendWebMessage(ToPlatformString(message));
    }

    EXPORTED void Photino_SetTransparentEnabled(Photino* instance, const bool enabled)
    {
        assert(instance);
        if (!instance) return;

        instance->SetTransparentEnabled(enabled);
    }

    EXPORTED void Photino_SetContextMenuEnabled(Photino* instance, const bool enabled)
    {
        assert(instance);
        if (!instance) return;

        instance->SetContextMenuEnabled(enabled);
    }

    EXPORTED void Photino_SetZoomEnabled(Photino* instance, const bool enabled)
    {
        assert(instance);
        if (!instance) return;

        instance->SetZoomEnabled(enabled);
    }

    EXPORTED void Photino_SetDevToolsEnabled(Photino* instance, const bool enabled)
    {
        assert(instance);
        if (!instance) return;

        instance->SetDevToolsEnabled(enabled);
    }

    EXPORTED void Photino_SetFullScreen(Photino* instance, const bool fullScreen)
    {
        assert(instance);
        if (!instance) return;

        instance->SetFullScreen(fullScreen);
    }

    EXPORTED void Photino_SetIconFile(Photino* instance, Utf8String filename)
    {
        assert(instance);
        if (!instance || !filename) return;

        instance->SetIconFile(ToPlatformString(filename));
    }

    EXPORTED void Photino_SetMaximized(Photino* instance, const bool maximized)
    {
        assert(instance);
        if (!instance) return;

        instance->SetMaximized(maximized);
    }

    EXPORTED void Photino_SetMaxSize(Photino* instance, const int width, const int height)
    {
        assert(instance);
        if (!instance) return;

        instance->SetMaxSize(width, height);
    }

    EXPORTED void Photino_SetMinimized(Photino* instance, const bool minimized)
    {
        assert(instance);
        if (!instance) return;

        instance->SetMinimized(minimized);
    }

    EXPORTED void Photino_SetMinSize(Photino* instance, const int width, const int height)
    {
        assert(instance);
        if (!instance) return;

        instance->SetMinSize(width, height);
    }

    EXPORTED void Photino_SetPosition(Photino* instance, const int x, const int y)
    {
        assert(instance);
        if (!instance) return;

        instance->SetPosition(x, y);
    }

    EXPORTED void Photino_SetResizable(Photino* instance, const bool resizable)
    {
        assert(instance);
        if (!instance) return;

        instance->SetResizable(resizable);
    }

    EXPORTED void Photino_SetSize(Photino* instance, const int width, const int height)
    {
        assert(instance);
        if (!instance) return;

        instance->SetSize(width, height);
    }

    EXPORTED void Photino_SetTitle(Photino* instance, Utf8String title)
    {
        assert(instance);
        if (!instance) return;

        instance->SetTitle(ToPlatformString(title));
    }

    EXPORTED void Photino_SetTopmost(Photino* instance, const bool topmost)
    {
        assert(instance);
        if (!instance) return;
        instance->SetTopmost(topmost);
    }

    EXPORTED void Photino_SetZoom(Photino* instance, const int zoom)
    {
        assert(instance);
        if (!instance) return;
        instance->SetZoom(zoom);
    }

    EXPORTED void Photino_ShowNotification(const Photino* instance, Utf8String title, Utf8String body)
    {
        assert(instance);
        if (!instance) return;

        instance->ShowNotification(ToPlatformString(title), ToPlatformString(body));

    }

    EXPORTED void Photino_WaitForExit(const Photino* instance)
    {
        assert(instance);
        if (!instance) return;

        instance->WaitForExit();
    }

    //Dialog
    EXPORTED char** Photino_ShowOpenFile(const Photino* instance, Utf8String title, Utf8String defaultPath, bool multiSelect, Utf8String* filters, int filterCount, int* resultCount)
    {
        assert(instance && resultCount);
        if (!instance || !resultCount)
            return nullptr;

        *resultCount = 0;

        const auto dialog = instance->GetDialog();
        assert(dialog);
        if (!dialog)
            return nullptr;

        auto platformFilters = ToPlatformStringList(filters, filterCount);

        auto result = dialog->ShowOpenFile(ToPlatformString(title), ToPlatformString(defaultPath), multiSelect, platformFilters);

        return CopyUtf8StringArray(result, resultCount);
    }

    EXPORTED char** Photino_ShowOpenFolder(const Photino* instance, Utf8String title, Utf8String defaultPath, bool multiSelect, int* resultCount)
    {
        assert(instance && resultCount);
        if (!instance || !resultCount)
            return nullptr;

        *resultCount = 0;

        const auto dialog = instance->GetDialog();
        assert(dialog);
        if (!dialog)
            return nullptr;

        auto result = dialog->ShowOpenFolder(ToPlatformString(title), ToPlatformString(defaultPath), multiSelect);

        return CopyUtf8StringArray(result, resultCount);

    }

    EXPORTED char* Photino_ShowSaveFile(const Photino* instance, Utf8String title, Utf8String defaultPath, Utf8String* filters, int filterCount, Utf8String defaultFileName)
    {
        assert(instance);
        if (!instance)
            return nullptr;

        const auto dialog = instance->GetDialog();
        assert(dialog);
        if (!dialog)
            return nullptr;

        auto platformFilters = ToPlatformStringList(filters, filterCount);

        auto result = dialog->ShowSaveFile(ToPlatformString(title), ToPlatformString(defaultPath), platformFilters, ToPlatformString(defaultFileName));

        if (result.empty())
            return nullptr;

        return CopyUtf8String(ToUtf8String(result));
    }

    EXPORTED DialogResult Photino_ShowMessage(const Photino* instance, Utf8String title, Utf8String text, DialogButtons buttons, DialogIcon icon)
    {
        assert(instance);
        if (!instance)
            return DialogResult::Cancel;

        const auto dialog = instance->GetDialog();
        assert(dialog);
        if (!dialog)
            return DialogResult::Cancel;

        return dialog->ShowMessage( ToPlatformString(title), ToPlatformString(text), buttons, icon);
    }

    //Callbacks
    EXPORTED bool Photino_AddCustomSchemeName(Photino* instance, Utf8String scheme)
    {
        assert(instance);
        if (!instance || !scheme) return false;
        return instance->AddCustomSchemeName(scheme);
    }

    EXPORTED void Photino_GetAllMonitors(Photino* instance, const GetAllMonitorsCallback callback)
    {
        assert(instance);
        if (!instance || !callback) return;
        instance->GetAllMonitors(callback);
    }

    EXPORTED void Photino_SetClosingCallback(Photino* instance, const ClosingCallback callback)
    {
        assert(instance);
        if (!instance || !callback) return;
        instance->SetClosingCallback(callback);
    }

    EXPORTED void Photino_SetClosedCallback(Photino* instance, const ClosedCallback callback)
    {
        assert(instance);
        if (!instance || !callback) return;
        instance->SetClosedCallback(callback);
    }

    EXPORTED void Photino_SetFocusInCallback(Photino* instance, const FocusInCallback callback)
    {
        assert(instance);
        if (!instance || !callback) return;
        instance->SetFocusInCallback(callback);
    }

    EXPORTED void Photino_SetFocusOutCallback(Photino* instance, const FocusOutCallback callback)
    {
        assert(instance);
        if (!instance || !callback) return;
        instance->SetFocusOutCallback(callback);
    }

    EXPORTED void Photino_SetMovedCallback(Photino* instance, const MovedCallback callback)
    {
        assert(instance);
        if (!instance || !callback) return;
        instance->SetMovedCallback(callback);
    }

    EXPORTED void Photino_SetResizedCallback(Photino* instance, const ResizedCallback callback)
    {
        assert(instance);
        if (!instance || !callback) return;
        instance->SetResizedCallback(callback);
    }

    EXPORTED void Photino_Invoke(const Photino* instance, const InvokeCallback callback)
    {
        assert(instance);
        if (!instance || !callback) return;

        instance->Invoke(callback);
    }

    EXPORTED void* Photino_AllocateMemory(int size)
    {
        return AllocateMemory(size);
    }

    EXPORTED void Photino_FreeMemory(void* value)
    {
        FreeMemory(value);
    }

    EXPORTED char* Photino_AllocateString(int size)
    {
        return AllocateString(size);
    }

    EXPORTED void Photino_FreeString(char* value)
    {
        FreeString(value);
    }

    EXPORTED void Photino_FreeStringArray(char** values, int count)
    {
        FreeStringArray(values, count);
    }
}
