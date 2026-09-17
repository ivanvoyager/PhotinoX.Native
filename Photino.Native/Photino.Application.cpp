#include "Photino.Application.h"
#include "Photino.Application.InitParams.h"
#include "Photino.Application.Notifications.h"

#include "Photino.h"
#include "Photino.Memory.h"

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <stdexcept>

using namespace PhotinoX::Native;

namespace
{
    thread_local bool g_hasDispatcherAccess = false;
}

PhotinoApplication& PhotinoApplication::Instance()
{
    static PhotinoApplication application;
    return application;
}

void PhotinoApplication::InitializeFromInitParams(const PhotinoApplicationInitParams* initParams)
{
    ValidateInitParams(initParams);

    InitializeOptions(initParams);
    InitializeCallbacks(initParams);
    InitializeNotificationCallbacks(initParams);
}

void PhotinoApplication::InitializeOptions(const PhotinoApplicationInitParams* initParams)
{
    options_.applicationName = ToPlatformString(initParams->Options.ApplicationName);
    options_.applicationIconPath = ToPlatformString(initParams->Options.ApplicationIconPath);
    options_.notificationRegistrationId = ToPlatformString(initParams->Options.NotificationRegistrationId);

    notificationsEnabled_.store(initParams->Options.NotificationsEnabled, std::memory_order_release);
}

void PhotinoApplication::InitializeCallbacks(const PhotinoApplicationInitParams* initParams)
{
    startupCallback_ = initParams->Callbacks.StartupHandler;
    shutdownRequestedCallback_ = initParams->Callbacks.ShutdownRequestedHandler;
    exitCallback_ = initParams->Callbacks.ExitHandler;
    windowCollectionChangedCallback_ = initParams->Callbacks.WindowCollectionChangedHandler;

    callbackState_ = initParams->Callbacks.CallbackState;
}

void PhotinoApplication::InitializeNotificationCallbacks(const PhotinoApplicationInitParams* initParams)
{
    notificationActivatedCallback_ = initParams->NotificationCallbacks.NotificationActivatedHandler;
    notificationActionActivatedCallback_ = initParams->NotificationCallbacks.NotificationActionActivatedHandler;
    notificationInputActivatedCallback_ = initParams->NotificationCallbacks.NotificationInputActivatedHandler;
    notificationDismissedCallback_ = initParams->NotificationCallbacks.NotificationDismissedHandler;
    notificationFailedCallback_ = initParams->NotificationCallbacks.NotificationFailedHandler;
}

void PhotinoApplication::Uninitialize()
{
    startupCallback_ = nullptr;
    shutdownRequestedCallback_ = nullptr;
    exitCallback_ = nullptr;
    windowCollectionChangedCallback_ = nullptr;

    callbackState_ = nullptr;

    notificationActivatedCallback_ = nullptr;
    notificationActionActivatedCallback_ = nullptr;
    notificationInputActivatedCallback_ = nullptr;
    notificationDismissedCallback_ = nullptr;
    notificationFailedCallback_ = nullptr;

    options_.applicationName.clear();
    options_.applicationIconPath.clear();
    options_.notificationRegistrationId.clear();
}

bool PhotinoApplication::IsRunning() const noexcept
{
    return isRunning_.load(std::memory_order_acquire);
}

bool PhotinoApplication::IsShuttingDown() const noexcept
{
    return isShuttingDown_.load(std::memory_order_acquire);
}

int PhotinoApplication::Run(const PhotinoApplicationInitParams* initParams)
{
    bool expected = false;
    if (!isRunning_.compare_exchange_strong(expected, true, std::memory_order_acq_rel))
        throw std::logic_error("The application is already running.");

    Photino::Register();

    InitializeFromInitParams(initParams);

    isShuttingDown_.store(false, std::memory_order_release);
    exitCode_.store(0, std::memory_order_release);

    if (notificationsEnabled_.load(std::memory_order_acquire))
        InitializeNotifications();

    auto stopRunning = [&]
    {
        assert(windows_.empty());

        isShuttingDown_.store(true, std::memory_order_release);
        UninitializeNotifications();
        Uninitialize();
        g_hasDispatcherAccess = false;
        isRunning_.store(false, std::memory_order_release);
    };

    g_hasDispatcherAccess = true;

    try
    {
        InvokeStartup();

        if (IsShuttingDown())
        {
            int exitCode = exitCode_.load(std::memory_order_acquire);
            exitCode = InvokeExit(exitCode);
            exitCode_.store(exitCode, std::memory_order_release);

            stopRunning();
            return exitCode;
        }

        int exitCode = RunCore();

        exitCode = InvokeExit(exitCode);

        exitCode_.store(exitCode, std::memory_order_release);

        stopRunning();

        return exitCode;
    }
    catch (...)
    {
        stopRunning();
        throw;
    }
}

void PhotinoApplication::NotifySessionEnding() noexcept
{
    isShuttingDown_.store(true, std::memory_order_release);
}

bool PhotinoApplication::HandleShutdownRequest(int exitCode, PhotinoShutdownRequestReason reason) noexcept
{
    if (IsShuttingDown())
        return false;

    if (InvokeShutdownRequested(reason))
        return false;

    exitCode_.store(exitCode, std::memory_order_release);
    isShuttingDown_.store(true, std::memory_order_release);

    return true;
}

void PhotinoApplication::Shutdown(int exitCode, bool force) noexcept
{
    if (force)
    {
        exitCode_.store(exitCode, std::memory_order_release);
        isShuttingDown_.store(true, std::memory_order_release);
    }

    ShutdownCore(exitCode, force);
}

bool PhotinoApplication::CheckAccess() const noexcept
{
    return g_hasDispatcherAccess;
}

void PhotinoApplication::GetNotificationsEnabled(bool* enabled) const
{
    if (!enabled) return;

    *enabled = notificationsEnabled_.load(std::memory_order_acquire);
}

void PhotinoApplication::SetNotificationsEnabled(bool enabled)
{
    notificationsEnabled_.store(enabled, std::memory_order_release);

    if (!IsRunning() || IsShuttingDown())
        return;

    if (enabled && !notificationsInitialized_.load(std::memory_order_acquire))
        InitializeNotifications();
}

/*  Contract:
    > 0  request accepted/tracked; callbacks may follow
      0  not shown by policy/state; no callback
     -1  invalid request / ABI / precondition failure; no callback
     -2  native notification backend initialization failure; no callback
     -3  native notification show failure; no callback
 */
int PhotinoApplication::ShowNotification(const PhotinoNotificationShowParams* showParams)
{
    if (!showParams)
        return -1;

    if (showParams->Size != sizeof(PhotinoNotificationShowParams) ||
        showParams->AbiVersion != PhotinoNotificationShowParams::NativeAbiVersion)
    {
        return -1;
    }

    if (!notificationsEnabled_.load(std::memory_order_acquire))
        return 0;

    if (!IsRunning() || IsShuttingDown())
        return 0;

    if (!notificationsInitialized_.load(std::memory_order_acquire))
    {
        if (!InitializeNotifications())
            return -2;
    }

    const auto title = ToPlatformString(showParams->Title);
    const auto body = ToPlatformString(showParams->Body);

    auto iconPath = ToPlatformString(showParams->IconPath);
    if (iconPath.empty())
        iconPath = options_.applicationIconPath;

    return ShowNotificationCore(showParams->NotificationId, title, body, iconPath, showParams->CallbackState);
}

bool PhotinoApplication::RegisterWindow(Photino* photino)
{
    assert(CheckAccess() || !IsRunning());
    assert(photino);
    if (!photino) return false;

    const auto iterator = std::find(windows_.begin(), windows_.end(), photino);
    assert(iterator == windows_.end());

   if (iterator != windows_.end())
        return true;

    windows_.push_back(photino);

    void* newItem = photino->CallbackState();

    return InvokeWindowCollectionChanged(NotifyCollectionChangedAction::Add, &newItem, 1, nullptr, 0);
}

void PhotinoApplication::UnregisterWindow(Photino* photino) noexcept
{
    assert(CheckAccess());
    assert(photino);
    if (!photino) return;

    const auto iterator = std::find(windows_.begin(), windows_.end(), photino);
    assert(iterator != windows_.end());

    if (iterator == windows_.end())
        return;

    void* oldItem = photino->CallbackState();

    windows_.erase(iterator);

    InvokeWindowCollectionChanged(NotifyCollectionChangedAction::Remove, nullptr, 0, &oldItem, 1);
}

bool PhotinoApplication::GetWindows(void** states, int* count) const
{
    assert(CheckAccess());
    assert(states);
    assert(count);

    if (!states || !count || !CheckAccess())
        return false;

    *states = nullptr;
    *count = 0;

    const auto windowCount = windows_.size();

    if (windowCount == 0)
        return true;

    const auto size = windowCount * sizeof(void*);

    auto values = static_cast<void**>(AllocateMemory(static_cast<int>(size)));

    if (!values)
        return false;

    for (std::size_t i = 0; i < windowCount; ++i)
    {
        values[i] = windows_[i]->CallbackState();
    }

    *states = values;
    *count = static_cast<int>(windowCount);

    return true;
}
