#pragma once

#include "Photino.Callbacks.h"
#include "Photino.Strings.h"

#include <type_traits>

namespace PhotinoX::Native
{
    struct PhotinoApplicationInitParams
    {
        static constexpr int NativeAbiVersion = 1;

        int Size;                                                               // #1
        int AbiVersion;                                                         // #2

        Utf8String ApplicationName;                                             // #3
        Utf8String ApplicationIconPath;                                         // #4
        Utf8String NotificationRegistrationId;                                  // #5

        StartupCallback StartupHandler;                                         // #6
        ExitCallback ExitHandler;                                               // #7

        NotificationActivatedCallback NotificationActivatedHandler;             // #8
        NotificationActionActivatedCallback NotificationActionActivatedHandler; // #9
        NotificationInputActivatedCallback NotificationInputActivatedHandler;   // #10
        NotificationDismissedCallback NotificationDismissedHandler;             // #11
        NotificationFailedCallback NotificationFailedHandler;                   // #12

        bool NotificationsEnabled;                                              // #13
    };

    static_assert(std::is_standard_layout_v<PhotinoApplicationInitParams>,
                  "PhotinoApplicationInitParams must remain standard-layout for managed/native interop.");

    static_assert(sizeof(PhotinoApplicationInitParams) == 96,
                  "PhotinoApplicationInitParams size changed. Update the managed ABI layout and size validation.");

} // namespace PhotinoX::Native