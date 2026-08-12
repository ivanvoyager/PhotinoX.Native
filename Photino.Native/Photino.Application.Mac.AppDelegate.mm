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
    auto& application = PhotinoX::Native::PhotinoApplication::Instance();

    if (!application.IsShuttingDown() && !application.HandleShutdownRequest(0, PhotinoShutdownRequestReason::Unknown))
        return NSTerminateCancel;

    application.StopApplicationLoop();
    return NSTerminateCancel;
}

- (void)applicationWillTerminate:(NSNotification*)notification
{
    PHOTINO_MAC_LOG("[mac-event] applicationWillTerminate\n");
}

@end
#endif