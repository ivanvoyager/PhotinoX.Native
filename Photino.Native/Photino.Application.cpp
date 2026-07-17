#include "Photino.Application.h"

#include <stdexcept>

using namespace PhotinoX::Native;

PhotinoApplication& PhotinoApplication::Instance()
{
    static PhotinoApplication application;
    return application;
}

bool PhotinoApplication::IsRunning() const noexcept
{
    return isRunning_.load(std::memory_order_acquire);
}

bool PhotinoApplication::IsShuttingDown() const noexcept
{
    return isShuttingDown_.load(std::memory_order_acquire);
}

int PhotinoApplication::Run()
{
    bool expected = false;
    if (!isRunning_.compare_exchange_strong(expected, true, std::memory_order_acq_rel))
        throw std::logic_error("The application is already running.");

    isShuttingDown_.store(false, std::memory_order_release);
    exitCode_.store(0, std::memory_order_release);

    try
    {
        int exitCode = RunCore();

        isShuttingDown_.store(true, std::memory_order_release);
        isRunning_.store(false, std::memory_order_release);

        return exitCode;
    }
    catch (...)
    {
        isShuttingDown_.store(true, std::memory_order_release);
        isRunning_.store(false, std::memory_order_release);
        throw;
    }
}

void PhotinoApplication::Shutdown(int exitCode) noexcept
{
    exitCode_.store(exitCode, std::memory_order_release);
    isShuttingDown_.store(true, std::memory_order_release);

    ShutdownCore(exitCode);
}