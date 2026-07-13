#pragma once

#ifdef __APPLE__

#include <Cocoa/Cocoa.h>

namespace PhotinoX::Native
{
    class Photino;
} // namespace PhotinoX::Native

@interface WindowDelegate : NSObject <NSWindowDelegate>
{
    @public
        PhotinoX::Native::Photino* photino;
}
@end

#endif