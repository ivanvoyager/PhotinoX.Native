#include "Photino.Application.h"

using namespace PhotinoX::Native;

int PhotinoApplication::RunCore()
{
    // TODO: Run GTK main loop.
    return exitCode_.load(std::memory_order_acquire);
}

void PhotinoApplication::ShutdownCore(int exitCode) noexcept
{

}

bool PhotinoApplication::Invoke(InvokeCallback callback) const
{
    // TODO: Dispatch through GLib main context.
    return false;
}

bool PhotinoApplication::BeginInvoke(InvokeCallback callback) const
{
    // TODO: Dispatch through GLib main context.
    return false;
}