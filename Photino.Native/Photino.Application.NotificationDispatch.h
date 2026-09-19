#pragma once

#include "Photino.Application.h"
#include "Photino.Enums.h"

#include <cassert>
#include <memory>
#include <string>
#include <utility>

namespace PhotinoX::Native::NotificationDispatch
{
    template <typename TState>
    void ReleaseState(void* value) noexcept
    {
        delete static_cast<TState*>(value);
    }

    struct NotificationState
    {
        PhotinoApplication* App;
        int NotificationId;
        void* CallbackState;
    };

    struct NotificationActionState
    {
        PhotinoApplication* App;
        int NotificationId;
        int ActionIndex;
        void* CallbackState;
    };

    struct NotificationInputState
    {
        PhotinoApplication* App;
        int NotificationId;
        std::string Response;
        void* CallbackState;
    };

    struct NotificationDismissedState
    {
        PhotinoApplication* App;
        int NotificationId;
        PhotinoNotificationDismissalReason Reason;
        void* CallbackState;
    };

    inline void InvokeNotificationActivated(void* value)
    {
        std::unique_ptr<NotificationState> state(static_cast<NotificationState*>(value));
        state->App->InvokeNotificationActivated(state->NotificationId, state->CallbackState);
    }

    inline void InvokeNotificationActionActivated(void* value)
    {
        std::unique_ptr<NotificationActionState> state(static_cast<NotificationActionState*>(value));
        state->App->InvokeNotificationActionActivated(state->NotificationId, state->ActionIndex, state->CallbackState);
    }

    inline void InvokeNotificationInputActivated(void* value)
    {
        std::unique_ptr<NotificationInputState> state(static_cast<NotificationInputState*>(value));
        state->App->InvokeNotificationInputActivated(state->NotificationId, state->Response.c_str(), state->CallbackState);
    }

    inline void InvokeNotificationDismissed(void* value)
    {
        std::unique_ptr<NotificationDismissedState> state(static_cast<NotificationDismissedState*>(value));
        state->App->InvokeNotificationDismissed(state->NotificationId, state->Reason, state->CallbackState);
    }

    inline void InvokeNotificationFailed(void* value)
    {
        std::unique_ptr<NotificationState> state(static_cast<NotificationState*>(value));
        state->App->InvokeNotificationFailed(state->NotificationId, state->CallbackState);
    }

    inline void ScheduleNotificationActivated(PhotinoApplication* app, int notificationId, void* callbackState)
    {
        assert(app);
        if (!app) return;

        auto state = new NotificationState{app, notificationId, callbackState};
        if (!app->BeginInvoke(InvokeNotificationActivated, ReleaseState<NotificationState>, state))
            delete state;
    }

    inline void ScheduleNotificationActionActivated(PhotinoApplication* app, int notificationId, int actionIndex, void* callbackState)
    {
        assert(app);
        if (!app) return;

        auto state = new NotificationActionState{app, notificationId, actionIndex, callbackState};
        if (!app->BeginInvoke(InvokeNotificationActionActivated, ReleaseState<NotificationActionState>, state))
            delete state;
    }

    inline void ScheduleNotificationInputActivated(PhotinoApplication* app, int notificationId, std::string response, void* callbackState)
    {
        assert(app);
        if (!app) return;

        auto state = new NotificationInputState{app, notificationId, std::move(response), callbackState};
        if (!app->BeginInvoke(InvokeNotificationInputActivated, ReleaseState<NotificationInputState>, state))
            delete state;
    }

    inline void ScheduleNotificationDismissed(PhotinoApplication* app, int notificationId, PhotinoNotificationDismissalReason reason, void* callbackState)
    {
        assert(app);
        if (!app) return;

        auto state = new NotificationDismissedState{app, notificationId, reason, callbackState};
        if (!app->BeginInvoke(InvokeNotificationDismissed, ReleaseState<NotificationDismissedState>, state))
            delete state;
    }

    inline void ScheduleNotificationFailed(PhotinoApplication* app, int notificationId, void* callbackState)
    {
        assert(app);
        if (!app) return;

        auto state = new NotificationState{app, notificationId, callbackState};
        if (!app->BeginInvoke(InvokeNotificationFailed, ReleaseState<NotificationState>, state))
            delete state;
    }
} // namespace PhotinoX::Native::NotificationDispatch
