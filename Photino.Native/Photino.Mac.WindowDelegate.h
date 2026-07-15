#pragma once

#ifdef __APPLE__

#import <AppKit/AppKit.h>

namespace PhotinoX::Native
{
    class Photino;
}

@interface WindowDelegate : NSObject <NSWindowDelegate>
{
    @public
        PhotinoX::Native::Photino* photino;
}
@end

#endif