#pragma once

#ifdef _WIN32

#include "Photino.Geometry.h"

#include <WebView2.h>
#include <Windows.h>
#include <wil/com.h>

namespace PhotinoX::Native
{
    class WinToastHandler;

    struct WindowsState
    {
        HWND hWnd = nullptr;

        WinToastHandler* toastHandler = nullptr;

        wil::com_ptr<ICoreWebView2Environment> webViewEnvironment;
        wil::com_ptr<ICoreWebView2Controller> webViewController;
        wil::com_ptr<ICoreWebView2> webViewWindow;

        PlatformString scriptId;

        WindowSizeLimits sizeLimits;

        bool suppressWindowCallbacks = true;
        bool webViewInitialized = false;

        int initialShowCommand = SW_SHOWDEFAULT;
        bool isAlreadyShown = false;

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