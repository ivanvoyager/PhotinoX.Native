#include "Photino.Application.h"

using namespace PhotinoX::Native;

int PhotinoApplication::RunCore()
{
    // TODO: Run Windows message loop.

    isRunning_.store(false, std::memory_order_release);
    isShuttingDown_.store(true, std::memory_order_release);
    return exitCode_.load(std::memory_order_acquire);
}

void PhotinoApplication::Invoke(InvokeCallback callback) const
{
    // TODO: Dispatch through hidden message window.
}

void PhotinoApplication::BeginInvoke(InvokeCallback callback) const
{
    // TODO: Dispatch through hidden message window.
}