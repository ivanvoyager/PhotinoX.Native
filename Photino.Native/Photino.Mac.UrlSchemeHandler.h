#ifdef __APPLE__
#pragma once
#include "Photino.h"

using namespace PhotinoX::Native;

@interface UrlSchemeHandler : NSObject <WKURLSchemeHandler> {
    @public
    WebResourceRequestedCallback requestHandler;
}
@end
#endif