#ifdef __APPLE__
#import "Photino.Mac.WindowDelegate.h"

#include "Photino.Application.h"
#include "Photino.h"
#include "Photino.Mac.State.h"

#include <dispatch/dispatch.h>

using namespace PhotinoX::Native;

//https://developer.apple.com/documentation/appkit/nswindowdelegate
@implementation WindowDelegate

- (id)init {
    self = [super init];
    if (self)
        photino = nullptr;

    return self;
}

- (void)windowDidResize:(NSNotification*)notification {
    if (!photino) return;

    int width = 0, height = 0;
    photino->GetSize(&width, &height);
    photino->InvokeResize(width, height);
}

- (void)windowDidMove:(NSNotification*)notification {
    if (!photino) return;

    int x = 0, y = 0;
    photino->GetPosition(&x, &y);
    photino->InvokeMove(x, y);
}

- (void)windowDidBecomeKey:(NSNotification*)notification {
    if (!photino) return;

    photino->InvokeFocusIn();
}

- (void)windowDidResignKey:(NSNotification*)notification {
    if (!photino) return;

    photino->InvokeFocusOut();
}

- (void)windowDidMiniaturize:(NSNotification*)notification {
    if (!photino) return;

    photino->InvokeMinimized();
}

- (void)windowDidDeminiaturize:(NSNotification*)notification {
    if (!photino) return;

    photino->InvokeRestored();
}

- (void)windowDidEnterFullScreen:(NSNotification*)notification {
    if (!photino) return;

    photino->HandleFullScreenStateChanged(true);
}

- (void)windowDidExitFullScreen:(NSNotification*)notification {
    if (!photino) return;

    photino->HandleFullScreenStateChanged(false);
    photino->InvokeRestored();
}

- (BOOL)windowShouldClose:(NSWindow*)sender {
    if (!photino) return YES;

    if (PhotinoApplication::Instance().IsShuttingDown()) return YES;

    bool doNotClose = photino->InvokeClosing();
    return doNotClose ? NO : YES;
}

- (void)windowWillClose:(NSNotification*)notification {
    if (!photino) return;

    Photino* instance = photino;
    photino = nullptr;

    instance->InvokeClose();

    NSWindow* window = (NSWindow*)notification.object;
    [window setDelegate:nil];

    dispatch_async(dispatch_get_main_queue(), ^{
        delete instance;
    });
}

@end

#endif