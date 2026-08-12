#pragma once

#include "Photino.Callbacks.h"
#include "Photino.Strings.h"

#include <type_traits>

namespace PhotinoX::Native
{
    struct PhotinoApplicationInitParams
    {
        static constexpr int NativeAbiVersion = 2;

        int Size;                                                               // #1
        int AbiVersion;                                                         // #2

        Utf8String ApplicationName;                                             // #3
        Utf8String ApplicationIconPath;                                         // #4
        Utf8String NotificationRegistrationId;                                  // #5

        StartupCallback StartupHandler;                                         // #6
        ShutdownRequestedCallback ShutdownRequestedHandler;                     // #7
        ExitCallback ExitHandler;                                               // #8

        NotificationActivatedCallback NotificationActivatedHandler;             // #9
        NotificationActionActivatedCallback NotificationActionActivatedHandler; // #10
        NotificationInputActivatedCallback NotificationInputActivatedHandler;   // #11
        NotificationDismissedCallback NotificationDismissedHandler;             // #12
        NotificationFailedCallback NotificationFailedHandler;                   // #13

        bool NotificationsEnabled;                                              // #14
    };

    static_assert(std::is_standard_layout_v<PhotinoApplicationInitParams>,
                  "PhotinoApplicationInitParams must remain standard-layout for managed/native interop.");

    static_assert(sizeof(PhotinoApplicationInitParams) == 104,
                  "PhotinoApplicationInitParams size changed. Update the managed ABI layout and size validation.");

} // namespace PhotinoX::Native