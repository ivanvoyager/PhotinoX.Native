#pragma once

#ifdef __APPLE__

#import <Foundation/Foundation.h>
#import <WebKit/WebKit.h>

namespace PhotinoX::Native
{
    class Photino;
}

@class NSWindow;

@interface NavigationDelegate : NSObject <WKNavigationDelegate>
{
    @public
        NSWindow* window;
        PhotinoX::Native::Photino* photino;
}
@end

#endif