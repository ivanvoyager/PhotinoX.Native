#include "Photino.Application.h"

#import <Cocoa/Cocoa.h>

#include <cassert>

using namespace PhotinoX::Native;

namespace
{
    void StopApplicationLoop()
    {
        [NSApp stop:nil];

        NSEvent* event = [NSEvent otherEventWithType:NSEventTypeApplicationDefined
                                            location:NSZeroPoint
                                       modifierFlags:0
                                           timestamp:0
                                        windowNumber:0
                                             context:nil
                                             subtype:0
                                               data1:0
                                               data2:0];

        [NSApp postEvent:event atStart:NO];
    }
}

int PhotinoApplication::RunCore()
{
    assert([NSThread isMainThread]);
    if (![NSThread isMainThread])
        return -1;

    @autoreleasepool
    {
        [NSApp run];
        return exitCode_.load(std::memory_order_acquire);
    }
}

void PhotinoApplication::ShutdownCore(int exitCode) noexcept
{
    exitCode_.store(exitCode, std::memory_order_release);

    if ([NSThread isMainThread])
    {
        StopApplicationLoop();
        return;
    }

    dispatch_async(dispatch_get_main_queue(), ^{
        StopApplicationLoop();
    });
}

bool PhotinoApplication::CheckAccess() const noexcept
{
    return [NSThread isMainThread];
}

bool PhotinoApplication::Invoke(InvokeStateCallback callback, void* state) const
{
    assert(callback);

    if (!callback || IsShuttingDown())
        return false;

    if (CheckAccess())
    {
        callback(state);
        return true;
    }

    if (!IsRunning())
        return false;

    dispatch_sync(dispatch_get_main_queue(), ^{
        callback(state);
    });

    return true;
}

bool PhotinoApplication::BeginInvoke(InvokeStateCallback callback, void* state) const
{
    assert(callback);

    if (!callback || IsShuttingDown() || !IsRunning())
        return false;

    dispatch_async(dispatch_get_main_queue(), ^{
        callback(state);
    });

    return true;
}