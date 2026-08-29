#pragma once

#include "Photino.Callbacks.h"
#include "Photino.Strings.h"

#include <type_traits>

namespace PhotinoX::Native
{
    struct PhotinoApplicationInitCallbacks
    {
        StartupCallback StartupHandler;                     // #1
        ShutdownRequestedCallback ShutdownRequestedHandler; // #2
        ExitCallback ExitHandler;                           // #3
        void* CallbackState;                                // #4
    };
    static_assert(std::is_standard_layout_v<PhotinoApplicationInitCallbacks>,
                  "PhotinoApplicationInitCallbacks must remain standard-layout for managed/native interop.");
    static_assert(sizeof(PhotinoApplicationInitCallbacks) == 32,
                  "PhotinoApplicationInitCallbacks size changed. Update the managed ABI layout and size validation.");

    struct PhotinoApplicationInitOptions
    {
        Utf8String ApplicationName;             // #1
        Utf8String ApplicationIconPath;         // #2
        Utf8String NotificationRegistrationId;  // #3

        bool NotificationsEnabled;              // #4
    };
    static_assert(std::is_standard_layout_v<PhotinoApplicationInitOptions>,
                  "PhotinoApplicationInitOptions must remain standard-layout for managed/native interop.");
    static_assert(sizeof(PhotinoApplicationInitOptions) == 32,
                  "PhotinoApplicationInitOptions size changed. Update the managed ABI layout and size validation.");

    struct PhotinoNotificationCallbacks
    {
        NotificationActivatedCallback NotificationActivatedHandler;             // #1
        NotificationActionActivatedCallback NotificationActionActivatedHandler; // #2
        NotificationInputActivatedCallback NotificationInputActivatedHandler;   // #3
        NotificationDismissedCallback NotificationDismissedHandler;             // #4
        NotificationFailedCallback NotificationFailedHandler;                   // #5
    };
    static_assert(std::is_standard_layout_v<PhotinoNotificationCallbacks>,
                  "PhotinoNotificationCallbacks must remain standard-layout for managed/native interop.");
    static_assert(sizeof(PhotinoNotificationCallbacks) == 40,
                  "PhotinoNotificationCallbacks size changed. Update the managed ABI layout and size validation.");

    struct PhotinoApplicationInitParams
    {
        static constexpr int NativeAbiVersion = 3;

        int Size;                                                               // #1
        int AbiVersion;                                                         // #2

        PhotinoApplicationInitCallbacks Callbacks;                              // #3
        PhotinoApplicationInitOptions Options;                                  // #4
        PhotinoNotificationCallbacks NotificationCallbacks;                     // #5
    };
    static_assert(std::is_standard_layout_v<PhotinoApplicationInitParams>,
                  "PhotinoApplicationInitParams must remain standard-layout for managed/native interop.");

    static_assert(offsetof(PhotinoApplicationInitParams, Callbacks) == 8, "PhotinoApplicationInitParams.Callbacks offset changed.");
    static_assert(offsetof(PhotinoApplicationInitParams, Options) == 40, "PhotinoApplicationInitParams.Options offset changed.");
    static_assert(offsetof(PhotinoApplicationInitParams, NotificationCallbacks) == 72, "PhotinoApplicationInitParams.NotificationCallbacks offset changed.");

    static_assert(sizeof(PhotinoApplicationInitParams) == 112,
                  "PhotinoApplicationInitParams size changed. Update the managed ABI layout and size validation.");

} // namespace PhotinoX::Native