#pragma once

#ifdef _WIN32

#include "Photino.Geometry.h"
#include "Photino.Windows.WebView2Environment.h"

#include <WebView2.h>
#include <Windows.h>
#include <wil/com.h>

namespace PhotinoX::Native
{
    struct WindowsState
    {
        HWND hWnd = nullptr;

        wil::com_ptr<ICoreWebView2Environment> webViewEnvironment;
        wil::com_ptr<ICoreWebView2Controller> webViewController;
        wil::com_ptr<ICoreWebView2> webViewWindow;

        WebView2EnvironmentSharingKey webView2EnvironmentSharingKey;
        WebView2EnvironmentKey webView2EnvironmentKey;

        PlatformString scriptId;

        HICON ownedSmallIcon = nullptr;
        HICON ownedBigIcon = nullptr;

        WindowSizeLimits sizeLimits;
        int initialShowCommand = SW_SHOWDEFAULT;

        bool isAlreadyShown = false;
        bool suppressWindowCallbacks = true;
        bool webViewInitialized = false;
        // Set once the first navigation has been issued. Until then there is no
        // document to reload, so the Set*Enabled methods must not reload: doing so
        // would discard a still-pending navigation.
        bool initialNavigationIssued = false;

        LONG_PTR fullScreenStyle = 0;
        WINDOWPLACEMENT fullScreenPlacement{sizeof(WINDOWPLACEMENT)};
        bool hasFullScreenRestoreState = false;
    };
}

#endif