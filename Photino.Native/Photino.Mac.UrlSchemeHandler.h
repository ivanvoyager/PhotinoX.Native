#pragma once

#ifdef __APPLE__

#include "Photino.Callbacks.h"

#include <Foundation/Foundation.h>
#include <WebKit/WebKit.h>

@interface UrlSchemeHandler : NSObject <WKURLSchemeHandler>
{
    @public
        PhotinoX::Native::WebResourceRequestedCallback requestHandler;
}
@end

#endif