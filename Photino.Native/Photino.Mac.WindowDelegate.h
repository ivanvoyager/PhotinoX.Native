#ifdef __APPLE__
#pragma once
#include "Photino.h"

using namespace PhotinoX::Native;

@interface WindowDelegate : NSObject <NSWindowDelegate>
{
    @public
        Photino * photino;
}
@end
#endif