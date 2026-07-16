#pragma once

#ifdef __APPLE__

@class NSWindow;
@class WKWebView;
@class WKWebViewConfiguration;

@class WindowDelegate;
@class UiDelegate;
@class NavigationDelegate;

namespace PhotinoX::Native
{
    class Photino;

    struct MacState
    {
        NSWindow* window = nil;

        WKWebView* webView = nil;
        WKWebViewConfiguration* webViewConfiguration = nil;

        WindowDelegate* windowDelegate = nil;
        UiDelegate* uiDelegate = nil;
        NavigationDelegate* navigationDelegate = nil;
    };
}

bool PhotinoMacIsShuttingDown();
void PhotinoMacSetShuttingDown(bool value);

void PhotinoMacStopMessageLoopIfOwner(PhotinoX::Native::Photino* owner);

#endif