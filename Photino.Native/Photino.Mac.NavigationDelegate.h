#pragma once

#ifdef __APPLE__

#include <Cocoa/Cocoa.h>
#include <WebKit/WebKit.h>

namespace PhotinoX::Native
{
    class Photino;
}

@interface NavigationDelegate : NSObject <WKNavigationDelegate>
{
    @public
        NSWindow* window;
        PhotinoX::Native::Photino* photino;
}
@end

#endif