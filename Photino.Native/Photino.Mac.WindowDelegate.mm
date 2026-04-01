#ifdef __APPLE__
#import "Photino.Mac.WindowDelegate.h"

//https://developer.apple.com/documentation/appkit/nswindowdelegate
@implementation WindowDelegate

- (void)windowDidResize:(NSNotification *)notification {
    int width, height;
    photino->GetSize(&width, &height);
    photino->InvokeResize(width, height);
}  

- (void)windowDidMove:(NSNotification *)notification {
    int x, y;
    photino->GetPosition(&x, &y);
    photino->InvokeMove(x, y);
}

- (void)windowDidBecomeKey:(NSNotification *)notification {
    photino->InvokeFocusIn();
}

- (void)windowDidResignKey:(NSNotification *)notification {
    photino->InvokeFocusOut();
}

- (void)windowDidMiniaturize:(NSNotification *)notification {
    photino->InvokeMinimized();
}

- (void)windowDidDeminiaturize:(NSNotification *)notification {
    photino->InvokeRestored();
}

- (BOOL)windowShouldClose:(NSWindow *)sender {
    bool doNotClose = photino->InvokeClosing();
    return doNotClose ? NO : YES;
}

- (void)windowWillClose:(NSNotification *)notification {
    photino->InvokeClose();
    NSWindow *window = (NSWindow *)notification.object;
    [window setDelegate:nil];
}

@end

#endif