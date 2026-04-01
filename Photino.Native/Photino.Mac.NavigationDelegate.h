#ifdef __APPLE__
#pragma once
#include "Photino.h"

using namespace PhotinoX::Native;

@interface NavigationDelegate: NSObject<WKNavigationDelegate>{
    @public
    NSWindow * window;
    Photino * photino;
}
@end
#endif