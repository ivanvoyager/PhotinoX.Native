#include "Photino.h"

#include <cassert>
#include <cstdlib>

using namespace PhotinoX::Native;

void Photino::InitializeFromInitParams(const PhotinoInitParams* initParams)
{
    assert(initParams);
    if (!initParams)
        std::abort();

    InitializeOptions(initParams);
    InitializeCallbacks(initParams);
    InitializeCustomSchemes(initParams);

    parent_ = initParams->ParentInstance;
}

void Photino::InitializeOptions(const PhotinoInitParams* initParams)
{
    options_.windowTitle = ToPlatformString(initParams->Title);
    options_.iconFileName = ToPlatformString(initParams->WindowIconFile);

    options_.startString = ToPlatformString(initParams->StartString);
    options_.startUrl = ToPlatformString(initParams->StartUrl);
    
    options_.temporaryFilesPath = ToPlatformString(initParams->TemporaryFilesPath);
    options_.userAgent = ToPlatformString(initParams->UserAgent);
    options_.browserControlInitParameters = ToPlatformString(initParams->BrowserControlInitParameters);
    options_.notificationRegistrationId = ToPlatformString(initParams->NotificationRegistrationId);

    options_.zoom = initParams->Zoom;
    options_.chromeless = initParams->Chromeless;
    options_.windowState = initParams->WindowState;

    options_.transparentEnabled = initParams->Transparent;
    options_.contextMenuEnabled = initParams->ContextMenuEnabled;
    options_.zoomEnabled = initParams->ZoomEnabled;
    options_.devToolsEnabled = initParams->DevToolsEnabled;
    options_.grantBrowserPermissions = initParams->GrantBrowserPermissions;
#if defined(_WIN32) || defined(__linux__)
    options_.mediaAutoplayEnabled = initParams->MediaAutoplayEnabled;
#endif
    options_.fileSystemAccessEnabled = initParams->FileSystemAccessEnabled;
    options_.webSecurityEnabled = initParams->WebSecurityEnabled;
    options_.javascriptClipboardAccessEnabled = initParams->JavascriptClipboardAccessEnabled;
    options_.mediaStreamEnabled = initParams->MediaStreamEnabled;
#if defined(_WIN32) || defined(__linux__)
    options_.smoothScrollingEnabled = initParams->SmoothScrollingEnabled;
#endif
    options_.ignoreCertificateErrorsEnabled = initParams->IgnoreCertificateErrorsEnabled;
    options_.notificationsEnabled = initParams->NotificationsEnabled;
}

void Photino::InitializeCallbacks(const PhotinoInitParams* initParams)
{
    closingCallback_ = initParams->ClosingHandler;
    focusInCallback_ = initParams->FocusInHandler;
    focusOutCallback_ = initParams->FocusOutHandler;
    resizedCallback_ = initParams->ResizedHandler;
    maximizedCallback_ = initParams->MaximizedHandler;
    restoredCallback_ = initParams->RestoredHandler;
    minimizedCallback_ = initParams->MinimizedHandler;
    movedCallback_ = initParams->MovedHandler;
    webMessageReceivedCallback_ = initParams->WebMessageReceivedHandler;
    customSchemeCallback_ = initParams->CustomSchemeHandler;
    closedCallback_ = initParams->ClosedHandler;
    fullScreenChangedCallback_ = initParams->FullScreenChangedHandler;
    stateChangedCallback_ = initParams->StateChangedHandler;
}

void Photino::InitializeCustomSchemes(const PhotinoInitParams* initParams)
{
    for (auto& customSchemeName : initParams->CustomSchemeNames)
    {
        AddCustomSchemeName(customSchemeName);
    }
}
