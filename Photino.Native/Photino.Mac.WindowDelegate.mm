#ifdef __APPLE__
#import "Photino.Mac.WindowDelegate.h"

#include "Photino.Application.h"
#include "Photino.h"
#include "Photino.Mac.State.h"
#include "Photino.Mac.Debug.h"

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

    PHOTINO_MAC_LOG("[mac-event] windowDidResize\n");

    if (!photino->IsFullScreenTransitioning())
        photino->UpdateWindowState();

    int width = 0, height = 0;
    photino->GetSize(&width, &height);
    photino->InvokeResize(width, height);
}

- (void)windowDidMove:(NSNotification*)notification {
    if (!photino) return;

    PHOTINO_MAC_LOG("[mac-event] windowDidMove\n");

    if (!photino->IsFullScreenTransitioning())
        photino->UpdateWindowState();

    int x = 0, y = 0;
    photino->GetPosition(&x, &y);
    photino->InvokeMove(x, y);
}

- (void)windowDidBecomeKey:(NSNotification*)notification {
    if (!photino) return;

    PHOTINO_MAC_LOG("[mac-event] windowDidBecomeKey\n");

    photino->InvokeFocusIn();
}

- (void)windowDidResignKey:(NSNotification*)notification {
    if (!photino) return;

    PHOTINO_MAC_LOG("[mac-event] windowDidResignKey\n");

    photino->InvokeFocusOut();
}

- (void)windowDidMiniaturize:(NSNotification*)notification {
    if (!photino) return;

    PHOTINO_MAC_LOG("[mac-event] windowDidMiniaturize\n");

    photino->HandleMiniaturizeCompleted();
}

- (void)windowDidDeminiaturize:(NSNotification*)notification {
    if (!photino) return;
    PHOTINO_MAC_LOG("[mac-event] windowDidDeminiaturize\n");

    photino->UpdateWindowState();
}

- (void)windowDidEnterFullScreen:(NSNotification*)notification {
    if (!photino) return;

    PHOTINO_MAC_LOG("[mac-event] windowDidEnterFullScreen\n");

    photino->SetFullScreenTransitioning(false);
    photino->UpdateWindowState();
}

- (void)windowDidExitFullScreen:(NSNotification*)notification {
    if (!photino) return;

    PHOTINO_MAC_LOG("[mac-event] windowDidExitFullScreen\n");

    photino->HandleFullScreenExitCompleted();
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