#pragma once

#include "Photino.Strings.h"

namespace PhotinoX::Native
{
    struct PhotinoApplicationOptions
    {
        PlatformString applicationName;
        PlatformString applicationIconPath;
        PlatformString notificationRegistrationId;
    };
} // namespace PhotinoX::Native