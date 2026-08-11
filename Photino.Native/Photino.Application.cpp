#include "Photino.Application.h"
#include "Photino.Application.InitParams.h"
#include "Photino.Application.Notifications.h"

#include <cassert>
#include <cstdlib>
#include <stdexcept>

using namespace PhotinoX::Native;

PhotinoApplication& PhotinoApplication::Instance()
{
    static PhotinoApplication application;
    return application;
}

void PhotinoApplication::InitializeFromInitParams(const PhotinoApplicationInitParams* initParams)
{
    ValidateInitParams(initParams);

    startupCallback_ = initParams->StartupHandler;
    exitCallback_ = initParams->ExitHandler;

    notificationActivatedCallback_ = initParams->NotificationActivatedHandler;
    notificationActionActivatedCallback_ = initParams->NotificationActionActivatedHandler;
    notificationInputActivatedCallback_ = initParams->NotificationInputActivatedHandler;
    notificationDismissedCallback_ = initParams->NotificationDismissedHandler;
    notificationFailedCallback_ = initParams->NotificationFailedHandler;

    options_.applicationName = ToPlatformString(initParams->ApplicationName);
    options_.applicationIconPath = ToPlatformString(initParams->ApplicationIconPath);
    options_.notificationRegistrationId = ToPlatformString(initParams->NotificationRegistrationId);

    notificationsEnabled_.store(initParams->NotificationsEnabled, std::memory_order_release);
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

    InitializeFromInitParams(initParams);

    isShuttingDown_.store(false, std::memory_order_release);
    exitCode_.store(0, std::memory_order_release);

    if (notificationsEnabled_.load(std::memory_order_acquire))
        InitializeNotifications();

    auto stopRunning = [&]
    {
        isShuttingDown_.store(true, std::memory_order_release);
        isRunning_.store(false, std::memory_order_release);
        UninitializeNotifications();
    };

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

void PhotinoApplication::Shutdown(int exitCode) noexcept
{
    exitCode_.store(exitCode, std::memory_order_release);
    isShuttingDown_.store(true, std::memory_order_release);

    ShutdownCore(exitCode);
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
            return 0;
    }

    const auto title = ToPlatformString(showParams->Title);
    const auto body = ToPlatformString(showParams->Body);

    auto iconPath = ToPlatformString(showParams->IconPath);
    if (iconPath.empty())
        iconPath = options_.applicationIconPath;

    return ShowNotificationCore(showParams->NotificationId, title, body, iconPath, showParams->CallbackState);
}