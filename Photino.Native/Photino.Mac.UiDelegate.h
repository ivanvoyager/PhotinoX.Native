#ifdef __APPLE__
#pragma once
#include "Photino.h"

using namespace PhotinoX::Native;

@interface UiDelegate : NSObject <WKUIDelegate, WKScriptMessageHandler> {
    @public
    NSWindow * window;
    Photino * photino;
    WebMessageReceivedCallback webMessageReceivedCallback;
}
@end
#endif