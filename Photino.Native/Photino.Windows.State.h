#pragma once

#ifdef _WIN32

#include <WebView2.h>
#include <Windows.h>
#include <wil/com.h>

namespace PhotinoX::Native
{
    struct WindowsState
    {
        HWND hWnd = nullptr;

        wil::com_ptr<ICoreWebView2Environment> environment;
        wil::com_ptr<ICoreWebView2Controller> controller;
        wil::com_ptr<ICoreWebView2> webview;
    };
}

#endif