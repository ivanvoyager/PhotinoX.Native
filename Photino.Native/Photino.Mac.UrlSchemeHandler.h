#pragma once

#ifdef __APPLE__

#import <Foundation/Foundation.h>
#import <WebKit/WebKit.h>

namespace PhotinoX::Native
{
    class Photino;
}

@interface UrlSchemeHandler : NSObject <WKURLSchemeHandler>
{
    @public
        PhotinoX::Native::Photino* photino;
}
@end

#endif