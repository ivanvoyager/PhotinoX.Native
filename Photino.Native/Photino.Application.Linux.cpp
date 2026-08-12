#include "Photino.Application.h"
#include "Photino.Application.InitParams.h"
#include "Photino.Application.Notifications.h"
#include "Photino.Application.NotificationDispatch.h"
#include "Photino.Application.Linux.State.h"
#include "Photino.Strings.h"

#include <algorithm>
#include <cassert>
#include <condition_variable>
#include <cstdlib>
#include <cstring>
#include <mutex>

#include <glib.h>
#include <gtk/gtk.h>
#include <libnotify/notify.h>

using namespace PhotinoX::Native;

namespace PhotinoX::Native
{
    struct LinuxNotificationState
    {
        PhotinoApplication* app = nullptr;
        int notificationId = 0;
        void* callbackState = nullptr;
        NotifyNotification* notification = nullptr;
        gulong closedHandler = 0;
        bool activated = false;
    };
} // namespace PhotinoX::Native

namespace
{
    struct InvokeWaitInfo
    {
        InvokeStateCallback callback = nullptr;
        void* state = nullptr;
        std::condition_variable completionNotifier;
        std::mutex mutex;
        bool isCompleted = false;
    };

    gboolean InvokeCallbackSync(gpointer data)
    {
        auto waitInfo = static_cast<InvokeWaitInfo*>(data);
        if (!waitInfo)
            return G_SOURCE_REMOVE;

        if (waitInfo->callback)
            waitInfo->callback(waitInfo->state);

        {
            std::lock_guard guard(waitInfo->mutex);
            waitInfo->isCompleted = true;
        }

        waitInfo->completionNotifier.notify_one();
        return G_SOURCE_REMOVE;
    }

    struct InvokeAsyncInfo
    {
        InvokeStateCallback callback = nullptr;
        void* state = nullptr;
    };

    gboolean InvokeCallbackAsync(gpointer data)
    {
        auto info = static_cast<InvokeAsyncInfo*>(data);
        if (!info)
            return G_SOURCE_REMOVE;

        if (info->callback)
            info->callback(info->state);

        return G_SOURCE_REMOVE;
    }

    void DestroyInvokeAsyncInfo(gpointer data)
    {
        delete static_cast<InvokeAsyncInfo*>(data);
    }

    gboolean ShutdownApplication(gpointer data)
    {
        gtk_main_quit();
        return G_SOURCE_REMOVE;
    }

    PhotinoNotificationDismissalReason ToDismissalReason(gint reason)
    {
        switch (reason)
        {
        case NOTIFY_CLOSED_REASON_EXPIRED:
            return PhotinoNotificationDismissalReason::TimedOut;

        case NOTIFY_CLOSED_REASON_DISMISSED:
            return PhotinoNotificationDismissalReason::UserCanceled;

        case NOTIFY_CLOSED_REASON_API_REQUEST:
            return PhotinoNotificationDismissalReason::ApplicationHidden;

        case NOTIFY_CLOSED_REASON_UNDEFIEND:
        default:
            return PhotinoNotificationDismissalReason::Unknown;
        }
    }

    void RemoveNotificationState(LinuxNotificationState* state) noexcept
    {
        if (!state || !state->app)
            return;

        auto& notifications = state->app->Platform().notifications;

        notifications.erase(
            std::remove(notifications.begin(), notifications.end(), state),
            notifications.end());
    }

    void DeleteNotificationState(LinuxNotificationState* state) noexcept
    {
        if (!state)
            return;

        RemoveNotificationState(state);

        state->app = nullptr;
        state->closedHandler = 0;

        if (state->notification)
        {
            g_object_unref(G_OBJECT(state->notification));
            state->notification = nullptr;
        }

        delete state;
    }

    void OnNotificationClosed(NotifyNotification* notification, gpointer data)
    {
        auto state = static_cast<LinuxNotificationState*>(data);
        if (!state)
            return;

        state->closedHandler = 0;

        if (state->app && !state->activated)
        {
            NotificationDispatch::ScheduleNotificationDismissed(
                state->app,
                state->notificationId,
                ToDismissalReason(notify_notification_get_closed_reason(notification)),
                state->callbackState);
        }

        DeleteNotificationState(state);
    }

    void OnNotificationActionInvoked(NotifyNotification* notification, char* action, gpointer data)
    {
        auto state = static_cast<LinuxNotificationState*>(data);
        if (!state || !state->app)
            return;

        if (action && std::strcmp(action, "default") == 0 && !state->activated)
        {
            state->activated = true;
            NotificationDispatch::ScheduleNotificationActivated(state->app, state->notificationId, state->callbackState);
        }
    }


} // namespace

PhotinoApplication::PhotinoApplication() : platform_(std::make_unique<LinuxApplicationState>())
{
}

PhotinoApplication::~PhotinoApplication() = default;

void PhotinoApplication::ValidateInitParams(const PhotinoApplicationInitParams* initParams)
{
    if (!initParams)
        std::abort();

    if (initParams->Size != sizeof(PhotinoApplicationInitParams) ||
        initParams->AbiVersion != PhotinoApplicationInitParams::NativeAbiVersion)
    {
        GtkWidget* dialog = gtk_message_dialog_new(
            nullptr,
            GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_CLOSE,
            "Application initial parameters ABI mismatch. Passed size: %i bytes, expected size: %zu bytes. Passed ABI version: %i, expected ABI version: %i.",
            initParams->Size,
            sizeof(PhotinoApplicationInitParams),
            initParams->AbiVersion,
            PhotinoApplicationInitParams::NativeAbiVersion);

        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        std::abort();
    }
}

int PhotinoApplication::RunCore()
{
    gtk_main();
    return exitCode_.load(std::memory_order_acquire);
}

void PhotinoApplication::ShutdownCore(int exitCode) noexcept
{
    exitCode_.store(exitCode, std::memory_order_release);

    g_main_context_invoke_full(
        g_main_context_default(),
        G_PRIORITY_DEFAULT,
        ShutdownApplication,
        nullptr,
        nullptr);
}

bool PhotinoApplication::CheckAccess() const noexcept
{
    return g_main_context_is_owner(g_main_context_default());
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

    InvokeWaitInfo waitInfo{};
    waitInfo.callback = callback;
    waitInfo.state = state;

    g_main_context_invoke_full(
        g_main_context_default(),
        G_PRIORITY_DEFAULT,
        InvokeCallbackSync,
        &waitInfo,
        nullptr);

    std::unique_lock lock(waitInfo.mutex);
    waitInfo.completionNotifier.wait(lock, [&waitInfo]
                                     { return waitInfo.isCompleted; });

    return true;
}

bool PhotinoApplication::BeginInvoke(InvokeStateCallback callback, void* state) const
{
    assert(callback);

    if (!callback || IsShuttingDown() || !IsRunning())
        return false;

    auto info = new InvokeAsyncInfo{ callback, state };

    const guint sourceId = g_idle_add_full(
               G_PRIORITY_DEFAULT,
               InvokeCallbackAsync,
               info,
               DestroyInvokeAsyncInfo);

    if (sourceId == 0)
    {
        delete info;
        return false;
    }

    return true;
}

bool PhotinoApplication::InitializeNotifications()
{
    bool expected = false;
    if (!notificationsInitialized_.compare_exchange_strong(expected, true, std::memory_order_acq_rel))
        return true;

    const auto appName = options_.applicationName.empty()
                             ? PlatformString("PhotinoX")
                             : options_.applicationName;

    if (!notify_init(appName.c_str()))
    {
        notificationsInitialized_.store(false, std::memory_order_release);
        return false;
    }

    return true;
}

void PhotinoApplication::UninitializeNotifications() noexcept
{
    std::vector<LinuxNotificationState*> notifications;
    notifications.swap(platform_->notifications);

    for (LinuxNotificationState* state : notifications)
    {
        if (!state)
            continue;

        state->app = nullptr;

        if (state->notification)
        {
            if (state->closedHandler)
            {
                g_signal_handler_disconnect(state->notification, state->closedHandler);
                state->closedHandler = 0;
            }

            GError* error = nullptr;
            notify_notification_close(state->notification, &error);
            if (error)
                g_error_free(error);

            g_object_unref(G_OBJECT(state->notification));
            state->notification = nullptr;
        }

        delete state;
    }

    if (notificationsInitialized_.exchange(false, std::memory_order_acq_rel))
        notify_uninit();
}

int PhotinoApplication::ShowNotificationCore(int notificationId, const PlatformString& title, const PlatformString& body, const PlatformString& iconPath, void* callbackState)
{
    NotifyNotification* notification = notify_notification_new(title.c_str(), body.c_str(), nullptr);

    if (!notification)
        return -1;

    if (!iconPath.empty())
    {
        notify_notification_set_hint_string(notification, "image-path", iconPath.c_str());
    }

    if (!options_.notificationRegistrationId.empty())
    {
        notify_notification_set_hint_string(notification, "desktop-entry", options_.notificationRegistrationId.c_str());
    }

    auto state = new LinuxNotificationState{this, notificationId, callbackState, notification, 0, false};

    state->closedHandler = g_signal_connect(notification, "closed", G_CALLBACK(OnNotificationClosed), state);

    notify_notification_add_action(notification, "default", "Open", NOTIFY_ACTION_CALLBACK(OnNotificationActionInvoked), state, nullptr);

    platform_->notifications.push_back(state);

    GError* error = nullptr;
    if (!notify_notification_show(notification, &error))
    {
        if (error)
            g_error_free(error);

        NotificationDispatch::ScheduleNotificationFailed(this, notificationId, callbackState);

        if (state->closedHandler)
        {
            g_signal_handler_disconnect(notification, state->closedHandler);
            state->closedHandler = 0;
        }

        DeleteNotificationState(state);

        return -1;
    }

    return notificationId;
}