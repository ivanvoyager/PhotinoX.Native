#pragma once

#include "Photino.Callbacks.h"
#include "Photino.Enums.h"
#include "Photino.Strings.h"
#include "Photino.Application.InitParams.h"
#include "Photino.Application.Options.h"
#include "Photino.Application.Notifications.h"

#include <atomic>
#include <memory>

namespace PhotinoX::Native
{
#ifdef _WIN32
    struct WindowsApplicationState;
#elif defined(__linux__)
    struct LinuxApplicationState;
#elif defined(__APPLE__)
    struct MacApplicationState;
#endif
    class PhotinoApplication final
    {
      private:
        StartupCallback startupCallback_ = nullptr;
        ExitCallback exitCallback_ = nullptr;

        NotificationActivatedCallback notificationActivatedCallback_ = nullptr;
        NotificationActionActivatedCallback notificationActionActivatedCallback_ = nullptr;
        NotificationInputActivatedCallback notificationInputActivatedCallback_ = nullptr;
        NotificationDismissedCallback notificationDismissedCallback_ = nullptr;
        NotificationFailedCallback notificationFailedCallback_ = nullptr;

        PhotinoApplicationOptions options_;

        std::atomic_bool isRunning_{false};
        std::atomic_bool isShuttingDown_{false};
        std::atomic_int exitCode_{0};

        std::atomic_bool notificationsInitialized_{false};
        std::atomic_bool notificationsEnabled_{true};

#ifdef _WIN32
        std::unique_ptr<WindowsApplicationState> platform_;
#elif defined(__linux__)
        std::unique_ptr<LinuxApplicationState> platform_;
#elif defined(__APPLE__)
        std::unique_ptr<MacApplicationState> platform_;
#endif
        PhotinoApplication();
        ~PhotinoApplication();

        void ValidateInitParams(const PhotinoApplicationInitParams* initParams);
        void InitializeFromInitParams(const PhotinoApplicationInitParams* initParams);

#ifdef __APPLE__
        bool IsAppBundleProcess() const;
#endif
        bool InitializeNotifications();
        void UninitializeNotifications() noexcept;
        int ShowNotificationCore(int notificationId, const PlatformString& title, const PlatformString& body, const PlatformString& iconPath, void* callbackState);

        int RunCore();
        void ShutdownCore(int exitCode) noexcept;
      public:
        static PhotinoApplication& Instance();

        PhotinoApplication(const PhotinoApplication&) = delete;
        PhotinoApplication& operator=(const PhotinoApplication&) = delete;

#ifdef _WIN32
        WindowsApplicationState& Platform() noexcept { return *platform_; }
        const WindowsApplicationState& Platform() const noexcept { return *platform_; }
#elif defined(__linux__)
        LinuxApplicationState& Platform() noexcept { return *platform_; }
        const LinuxApplicationState& Platform() const noexcept { return *platform_; }
#elif defined(__APPLE__)
        MacApplicationState& Platform() noexcept { return *platform_; }
        const MacApplicationState& Platform() const noexcept { return *platform_; }
#endif

        bool IsRunning() const noexcept;
        bool IsShuttingDown() const noexcept;

        int Run(const PhotinoApplicationInitParams* initParams);
        void Shutdown(int exitCode = 0) noexcept;
        bool CheckAccess() const noexcept;

        bool Invoke(InvokeStateCallback callback, void* state) const;
        bool BeginInvoke(InvokeStateCallback callback, void* state) const;

        int ShowNotification(const PhotinoNotificationShowParams* showParams);

        void GetNotificationsEnabled(bool* enabled) const;
        void SetNotificationsEnabled(bool enabled);

        // Callbacks
        void SetStartupCallback(StartupCallback callback) noexcept { startupCallback_ = callback; }
        void SetExitCallback(ExitCallback callback) noexcept { exitCallback_ = callback; }

        void SetNotificationActivatedCallback(NotificationActivatedCallback callback) noexcept { notificationActivatedCallback_ = callback; }
        void SetNotificationActionActivatedCallback(NotificationActionActivatedCallback callback) noexcept { notificationActionActivatedCallback_ = callback; }
        void SetNotificationInputActivatedCallback(NotificationInputActivatedCallback callback) noexcept { notificationInputActivatedCallback_ = callback; }
        void SetNotificationDismissedCallback(NotificationDismissedCallback callback) noexcept { notificationDismissedCallback_ = callback; }
        void SetNotificationFailedCallback(NotificationFailedCallback callback) noexcept { notificationFailedCallback_ = callback; }

        // Callback invokers
        void InvokeStartup() const;
        int InvokeExit(int exitCode) const;

        void InvokeNotificationActivated(int notificationId, void* state) const;
        void InvokeNotificationActionActivated(int notificationId, int actionIndex, void* state) const;
        void InvokeNotificationInputActivated(int notificationId, Utf8String response, void* state) const;
        void InvokeNotificationDismissed(int notificationId, PhotinoNotificationDismissalReason reason, void* state) const;
        void InvokeNotificationFailed(int notificationId, void* state) const;

    };
} // namespace PhotinoX::Native
