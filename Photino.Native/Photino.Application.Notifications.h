#pragma once

#include "Photino.Strings.h"

#include <type_traits>

namespace PhotinoX::Native
{
    struct PhotinoNotificationShowParams
    {
        static constexpr int NativeAbiVersion = 1;

        int Size;
        int AbiVersion;

        int NotificationId;

        Utf8String Title;
        Utf8String Body;
        Utf8String IconPath;

        void* CallbackState;
    };

    static_assert(std::is_standard_layout_v<PhotinoNotificationShowParams>,
                  "PhotinoNotificationShowParams must remain standard-layout for managed/native interop.");

    static_assert(sizeof(PhotinoNotificationShowParams) == 48,
                  "PhotinoNotificationShowParams size changed. Update the managed ABI layout and size validation.");
} // namespace PhotinoX::Native