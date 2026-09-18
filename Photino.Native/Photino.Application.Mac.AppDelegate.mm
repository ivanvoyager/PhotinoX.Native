#ifdef __APPLE__
#import "Photino.Application.Mac.AppDelegate.h"

#include "Photino.Application.h"
#include "Photino.Mac.Debug.h"

using namespace PhotinoX::Native;

@implementation AppDelegate

- (void)applicationDidFinishLaunching:(NSNotification*)notification
{
    [NSApp activateIgnoringOtherApps:YES];
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication*)sender
{
    return NO;
}

- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication*)sender
{
    auto& application = PhotinoApplication::Instance();

    if (application.IsShuttingDown())
        return NSTerminateCancel;

    if (application.InvokeShutdownRequested(PhotinoShutdownRequestReason::Unknown))
        return NSTerminateCancel;

    application.HandleShutdown(0, true);
    return NSTerminateCancel;
}

- (void)applicationWillTerminate:(NSNotification*)notification
{
    PHOTINO_MAC_LOG("[mac-event] applicationWillTerminate\n");
}

@end
#endif