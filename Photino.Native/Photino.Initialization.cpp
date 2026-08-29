#include "Photino.InitParams.h"
#include "Photino.h"

#include <cassert>
#include <cstdlib>

using namespace PhotinoX::Native;

void Photino::InitializeFromInitParams(const PhotinoInitParams* initParams)
{
    assert(initParams);
    if (!initParams)
        std::abort();

    if (initParams->Size != sizeof(PhotinoInitParams) ||
        initParams->AbiVersion != PhotinoInitParams::NativeAbiVersion)
    {
        std::abort();
    }

    InitializeOptions(initParams);
    InitializeCallbacks(initParams);
    InitializeCustomSchemes(initParams);

    parent_ = initParams->ParentInstance;
}

void Photino::InitializeOptions(const PhotinoInitParams* initParams)
{
    // Window
    options_.windowTitle = ToPlatformString(initParams->Window.Title);
    options_.iconFileName = ToPlatformString(initParams->Window.IconFile);

    options_.chromeless = initParams->Window.Chromeless;
    options_.transparentEnabled = initParams->Window.Transparent;

    // Geometry
    options_.windowState = initParams->Geometry.WindowState;
    options_.resizable = initParams->Geometry.Resizable;

    // Browser
    options_.startString = ToPlatformString(initParams->Browser.StartString);
    options_.startUrl = ToPlatformString(initParams->Browser.StartUrl);

    options_.userAgent = ToPlatformString(initParams->Browser.UserAgent);
    options_.browserControlInitParameters = ToPlatformString(initParams->Browser.ControlInitParameters);
    options_.userDataFolder = ToPlatformString(initParams->Browser.UserDataFolder);

    options_.zoom = initParams->Browser.Zoom;
    options_.zoomEnabled = initParams->Browser.ZoomEnabled;
    options_.contextMenuEnabled = initParams->Browser.ContextMenuEnabled;
    options_.statusBarEnabled = initParams->Browser.StatusBarEnabled;
    options_.devToolsEnabled = initParams->Browser.DevToolsEnabled;

    options_.grantBrowserPermissions = initParams->Browser.GrantBrowserPermissions;
#if defined(_WIN32) || defined(__linux__)
    options_.mediaAutoplayEnabled = initParams->Browser.MediaAutoplayEnabled;
#endif
    options_.fileSystemAccessEnabled = initParams->Browser.FileSystemAccessEnabled;
    options_.webSecurityEnabled = initParams->Browser.WebSecurityEnabled;
    options_.javascriptClipboardAccessEnabled = initParams->Browser.JavascriptClipboardAccessEnabled;
    options_.mediaStreamEnabled = initParams->Browser.MediaStreamEnabled;
#if defined(_WIN32) || defined(__linux__)
    options_.smoothScrollingEnabled = initParams->Browser.SmoothScrollingEnabled;
#endif
    options_.ignoreCertificateErrorsEnabled = initParams->Browser.IgnoreCertificateErrorsEnabled;
}

void Photino::InitializeCallbacks(const PhotinoInitParams* initParams)
{
    closingCallback_ = initParams->Callbacks.ClosingHandler;
    closedCallback_ = initParams->Callbacks.ClosedHandler;
    focusInCallback_ = initParams->Callbacks.FocusInHandler;
    focusOutCallback_ = initParams->Callbacks.FocusOutHandler;
    resizedCallback_ = initParams->Callbacks.ResizedHandler;
    movedCallback_ = initParams->Callbacks.MovedHandler;
    maximizedCallback_ = initParams->Callbacks.MaximizedHandler;
    restoredCallback_ = initParams->Callbacks.RestoredHandler;
    minimizedCallback_ = initParams->Callbacks.MinimizedHandler;
    fullScreenChangedCallback_ = initParams->Callbacks.FullScreenChangedHandler;
    stateChangedCallback_ = initParams->Callbacks.StateChangedHandler;
    webMessageReceivedCallback_ = initParams->Callbacks.WebMessageReceivedHandler;
    customSchemeCallback_ = initParams->Callbacks.CustomSchemeHandler;
    navigationStartingCallback_ = initParams->Callbacks.NavigationStartingHandler;
    newWindowRequestedCallback_ = initParams->Callbacks.NewWindowRequestedHandler;
    contentLoadingCallback_ = initParams->Callbacks.ContentLoadingHandler;
    contentLoadedCallback_ = initParams->Callbacks.ContentLoadedHandler;

    callbackState_ = initParams->Callbacks.CallbackState;
}

void Photino::InitializeCustomSchemes(const PhotinoInitParams* initParams)
{
    for (auto& customSchemeName : initParams->Browser.CustomSchemeNames)
    {
        AddCustomSchemeName(customSchemeName);
    }
}
