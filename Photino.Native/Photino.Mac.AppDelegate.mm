#ifdef __APPLE__
#import "Photino.Mac.AppDelegate.h"

#include "Photino.Application.h"

@implementation AppDelegate

- (void)applicationDidFinishLaunching:(NSNotification*)notification
{
    [NSApp activateIgnoringOtherApps:YES];
    // NSLog(@"applicationDidFinishLaunching fired!");
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication*)sender
{
    //NSLog(@"applicationShouldTerminateAfterLastWindowClosed fired!");
    return NO;
}

- (void)applicationWillTerminate:(NSNotification*)notification
{
    PhotinoX::Native::PhotinoApplication::Instance().Shutdown();
}

@end
#endif