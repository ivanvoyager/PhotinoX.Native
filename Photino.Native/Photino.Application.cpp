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

    shutdownCompleted_.store(false, std::memory_order_release);
    isShuttingDown_.store(false, std::memory_order_release);
    isInMainLoop_.store(false, std::memory_order_release);
    exitCode_.store(0, std::memory_order_release);

    if (notificationsEnabled_.load(std::memory_order_acquire))
        InitializeNotifications();

    auto stopRunning = [&]
    {
        assert(windows_.empty());

        isShuttingDown_.store(true, std::memory_order_release);
        ReleasePendingInvokes();
        UninitializeNotifications();
        Uninitialize();
        g_hasDispatcherAccess = false;
        isRunning_.store(false, std::memory_order_release);
    };

    g_hasDispatcherAccess = true;

    bool applicationLoopInitialized = false;
    try
    {
        if (!InitializeCore())
        {
            isShuttingDown_.store(true, std::memory_order_release);
            CloseWindows();
            stopRunning();
            return -1;
        }
        applicationLoopInitialized = true;

        InvokeStartup();

        if (IsShuttingDown() && windows_.empty())
        {
            int exitCode = exitCode_.load(std::memory_order_acquire);
            exitCode = InvokeExit(exitCode);
            exitCode_.store(exitCode, std::memory_order_release);

            UninitializeCore();
            applicationLoopInitialized = false;

            stopRunning();
            return exitCode;
        }

        isInMainLoop_.store(true, std::memory_order_release);
        RequestPendingInvokes();

        int exitCode = RunCore();
        isInMainLoop_.store(false, std::memory_order_release);

        exitCode = InvokeExit(exitCode);
        exitCode_.store(exitCode, std::memory_order_release);

        UninitializeCore();
        applicationLoopInitialized = false;

        stopRunning();

        return exitCode;
    }
    catch (...)
    {
        isInMainLoop_.store(false, std::memory_order_release);

        if (!windows_.empty())
        {
            isShuttingDown_.store(true, std::memory_order_release);
            CloseWindows();
        }

        if (applicationLoopInitialized)
            UninitializeCore();

        stopRunning();
        throw;
    }
}

void PhotinoApplication::NotifySessionEnding() noexcept
{
    isShuttingDown_.store(true, std::memory_order_release);
}

void PhotinoApplication::Shutdown(int exitCode, bool force) noexcept
{
    if (!IsRunning())
        return;

    if (CheckAccess())
    {
        HandleShutdown(exitCode, force);
        return;
    }

    RequestShutdownCore(exitCode, force);
}

void PhotinoApplication::HandleShutdown(int exitCode, bool force) noexcept
{
    assert(CheckAccess());

    if (!CheckAccess() || IsShuttingDown())
        return;

    if (!force && InvokeShutdownRequested(PhotinoShutdownRequestReason::Application))
        return;

    exitCode_.store(exitCode, std::memory_order_release);
    isShuttingDown_.store(true, std::memory_order_release);

    CloseWindows();

    if (windows_.empty())
        CompleteShutdown();
}

void PhotinoApplication::CloseWindows() noexcept
{
    assert(CheckAccess());

    const auto windows = windows_;

    for (auto iterator = windows.rbegin(); iterator != windows.rend(); ++iterator)
    {
        if (*iterator)
            (*iterator)->Close();
    }
}

void PhotinoApplication::CompleteShutdown() noexcept
{
    assert(CheckAccess());
    assert(IsShuttingDown());
    assert(windows_.empty());

    if (!isInMainLoop_.load(std::memory_order_acquire))
        return;

    bool expected = false;
    if (!shutdownCompleted_.compare_exchange_strong(expected, true, std::memory_order_acq_rel))
        return;

    CompleteShutdownCore(exitCode_.load(std::memory_order_acquire));
}

bool PhotinoApplication::CheckAccess() const noexcept
{
    return g_hasDispatcherAccess;
}

bool PhotinoApplication::BeginInvoke(InvokeStateCallback callback, ReleaseStateCallback release, void* state) noexcept
{
    assert(callback);
    assert(release);

    if (!callback || !release || !IsRunning() || IsShuttingDown())
        return false;

    try
    {
        std::lock_guard lock(pendingInvokesMutex_);

        if (!IsRunning() || IsShuttingDown())
            return false;

        pendingInvokes_.push_back({callback, release, state});
    }
    catch (...)
    {
        return false;
    }

    if (isInMainLoop_.load(std::memory_order_acquire))
        RequestPendingInvokesCore();

    return true;
}

void PhotinoApplication::ProcessPendingInvokes() noexcept
{
    assert(CheckAccess());

    if (!CheckAccess())
        return;

    std::deque<PendingInvoke> pendingInvokes;

    {
        std::lock_guard lock(pendingInvokesMutex_);
        pendingInvokes.swap(pendingInvokes_);
    }

    auto iterator = pendingInvokes.begin();

    for (; iterator != pendingInvokes.end(); ++iterator)
    {
        if (IsShuttingDown())
            break;

        iterator->callback(iterator->state);
    }

    for (; iterator != pendingInvokes.end(); ++iterator)
        iterator->release(iterator->state);

    if (!IsShuttingDown())
        RequestPendingInvokes();
}

void PhotinoApplication::RequestPendingInvokes() noexcept
{
    bool hasPendingInvokes;
    {
        std::lock_guard lock(pendingInvokesMutex_);
        hasPendingInvokes = !pendingInvokes_.empty();
    }

    if (hasPendingInvokes)
        RequestPendingInvokesCore();
}

void PhotinoApplication::ReleasePendingInvokes() noexcept
{
    std::deque<PendingInvoke> pendingInvokes;
    {
        std::lock_guard lock(pendingInvokesMutex_);
        pendingInvokes.swap(pendingInvokes_);
    }

    for (const auto& pendingInvoke : pendingInvokes)
        pendingInvoke.release(pendingInvoke.state);
}

void PhotinoApplication::GetNotificationsEnabled(bool* enabled) const noexcept
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

    if (IsShuttingDown() && windows_.empty())
        CompleteShutdown();
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
