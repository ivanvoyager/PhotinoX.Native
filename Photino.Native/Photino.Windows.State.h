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

        bool webViewInitialized = false;
        bool isAlreadyShown = false;
    };
}

#endif