#ifdef __APPLE__
#import "Photino.Mac.AppDelegate.h"

#include "Photino.Application.h"

@implementation AppDelegate

- (void)applicationDidFinishLaunching:(NSNotification*)notification
{
    [NSApp activateIgnoringOtherApps:YES];
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication*)sender
{
    return NO;
}

- (void)applicationWillTerminate:(NSNotification*)notification
{
    PhotinoX::Native::PhotinoApplication::Instance().Shutdown();
}

@end
#endif