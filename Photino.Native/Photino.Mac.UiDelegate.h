#pragma once

#ifdef __APPLE__

#include "Photino.Callbacks.h"

#include <Cocoa/Cocoa.h>
#include <WebKit/WebKit.h>

namespace PhotinoX::Native
{
    class Photino;
}

@interface UiDelegate : NSObject <WKUIDelegate, WKScriptMessageHandler>
{
    @public
        NSWindow* window;
        PhotinoX::Native::Photino* photino;
        PhotinoX::Native::WebMessageReceivedCallback webMessageReceivedCallback;
}
@end

#endif