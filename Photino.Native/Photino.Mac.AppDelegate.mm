#ifdef __APPLE__
#import "Photino.Mac.AppDelegate.h"

#include "Photino.Mac.State.h"

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
    PhotinoMacSetShuttingDown(true);
}

@end
#endif