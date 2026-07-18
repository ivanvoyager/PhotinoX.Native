#pragma once

#include "Photino.Callbacks.h"

#include <atomic>

namespace PhotinoX::Native
{
class PhotinoApplication final
{
  private:
    std::atomic_bool isRunning_{false};
    std::atomic_bool isShuttingDown_{false};
    std::atomic_int exitCode_{0};

    PhotinoApplication() = default;
    int RunCore();
    void ShutdownCore(int exitCode) noexcept;
  public:
    static PhotinoApplication& Instance();

    PhotinoApplication(const PhotinoApplication&) = delete;
    PhotinoApplication& operator=(const PhotinoApplication&) = delete;

    bool IsRunning() const noexcept;
    bool IsShuttingDown() const noexcept;

    int Run();
    void Shutdown(int exitCode = 0) noexcept;
    bool CheckAccess() const noexcept;

    bool Invoke(InvokeCallback callback) const;
    bool BeginInvoke(InvokeStateCallback callback, void* state) const;
};
} // namespace PhotinoX::Native
