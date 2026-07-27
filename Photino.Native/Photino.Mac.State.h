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

        bool isFullScreenTransitioning = false;
        PhotinoWindowState pendingStateAfterFullScreenExit = PhotinoWindowState::Normal;
        PhotinoWindowState stateBeforeMinimize = PhotinoWindowState::Normal;
    };
}

#endif