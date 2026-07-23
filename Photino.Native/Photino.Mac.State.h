#pragma once

#ifdef __APPLE__

#include "Photino.Enums.h"

@class NSWindow;
@class WKWebView;
@class WKWebViewConfiguration;

@class WindowDelegate;
@class UiDelegate;
@class NavigationDelegate;

namespace PhotinoX::Native
{
    struct MacState
    {
        NSWindow* window = nil;

        WKWebView* webView = nil;
        WKWebViewConfiguration* webViewConfiguration = nil;

        WindowDelegate* windowDelegate = nil;
        UiDelegate* uiDelegate = nil;
        NavigationDelegate* navigationDelegate = nil;

        PhotinoWindowState pendingStateAfterFullScreenExit = PhotinoWindowState::Normal;
        bool isFullScreenTransitioning = false;
    };
}

#endif