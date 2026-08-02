#pragma once

#ifdef __APPLE__

#include "Photino.Callbacks.h"

#import <Foundation/Foundation.h>
#import <WebKit/WebKit.h>

namespace PhotinoX::Native
{
    class Photino;
}

@class NSWindow;

@interface UiDelegate : NSObject <WKUIDelegate, WKScriptMessageHandler>
{
    @public
        NSWindow* window;
        PhotinoX::Native::Photino* photino;
}
@end

#endif