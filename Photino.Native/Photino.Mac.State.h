#pragma once

#ifdef __APPLE__

#import <AppKit/AppKit.h>

#include "Photino.Enums.h"

@class WKWebView;
@class WKWebViewConfiguration;

@class WindowDelegate;
@class UiDelegate;
@class NavigationDelegate;

namespace PhotinoX::Native
{
    struct MacDragState
    {
        id monitor = nil;
        NSPoint startMouse{};
        NSPoint startOrigin{};
    };

    struct MacResizeState
    {
        id monitor = nil;
        NSPoint startMouse{};
        NSRect startFrame{};
        PhotinoWindowEdge edge{};
    };

    struct MacState
    {
        NSWindow* window = nil;

        WKWebView* webView = nil;
        WKWebViewConfiguration* webViewConfiguration = nil;

        WindowDelegate* windowDelegate = nil;
        UiDelegate* uiDelegate = nil;
        NavigationDelegate* navigationDelegate = nil;

        MacDragState drag;
        MacResizeState resize;

        bool isFullScreenTransitioning = false;
        bool logicalMaximized = false;
        bool hasNormalFrame = false;
        NSRect normalFrame{};

        PhotinoWindowState pendingStateAfterFullScreenExit = PhotinoWindowState::Normal;
        PhotinoWindowState stateBeforeMinimize = PhotinoWindowState::Normal;
    };
}

#endif