
#include "Photino.Application.h"

using namespace PhotinoX::Native;

int PhotinoApplication::RunCore()
{
    // TODO: Run NSApplication loop.
    isRunning_.store(false, std::memory_order_release);
    isShuttingDown_.store(true, std::memory_order_release);
    return exitCode_.load(std::memory_order_acquire);
}

void PhotinoApplication::Invoke(InvokeCallback callback) const
{
    // TODO: Dispatch through main dispatch queue.
}

void PhotinoApplication::BeginInvoke(InvokeCallback callback) const
{
    // TODO: Dispatch through main dispatch queue.
}