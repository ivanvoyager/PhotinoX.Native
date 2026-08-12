#pragma once

namespace PhotinoX::Native
{
    // Shutdown request reason. The numeric values are the ABI contract with
    // the managed PhotinoShutdownRequestReason enum and must stay in sync.
    enum class PhotinoShutdownRequestReason : int
    {
        Unknown = 0,
        Application = 1,
        SessionLogoff = 2,
        SystemShutdown = 3
    };

    static_assert(sizeof(PhotinoShutdownRequestReason) == sizeof(int),
                  "PhotinoShutdownRequestReason must remain int-sized for managed/native interop.");

    // Which edge or corner a resize drag operates on. The ordering is the ABI
    // contract with the managed PhotinoWindowEdge enum and must stay in sync.
    enum class PhotinoWindowEdge : int
    {
        Top,
        Bottom,
        Left,
        Right,
        TopLeft,
        TopRight,
        BottomLeft,
        BottomRight
    };

    static_assert(sizeof(PhotinoWindowEdge) == sizeof(int),
                  "PhotinoWindowEdge must remain int-sized for managed/native interop.");

    // Native window state. The ordering is the ABI contract with the managed
    // PhotinoWindowState enum and must stay in sync.
    enum class PhotinoWindowState : int
    {
        Normal,
        Minimized,
        Maximized,
        FullScreen
    };

    static_assert(sizeof(PhotinoWindowState) == sizeof(int),
                  "PhotinoWindowState must remain int-sized for managed/native interop.");

    // Notification dismissal reason. The numeric values are the ABI contract with
    // the managed NotificationDismissalReason enum and must stay in sync.
    enum class PhotinoNotificationDismissalReason : int
    {
        Unknown = 0,
        UserCanceled = 1,
        ApplicationHidden = 2,
        TimedOut = 3
    };
    static_assert(sizeof(PhotinoNotificationDismissalReason) == sizeof(int),
                  "PhotinoNotificationDismissalReason must remain int-sized for managed/native interop.");
}