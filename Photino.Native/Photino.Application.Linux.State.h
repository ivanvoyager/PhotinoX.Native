#pragma once

#ifdef __linux__

#include <vector>

namespace PhotinoX::Native
{
    struct LinuxNotificationState;

    struct LinuxApplicationState
    {
        std::vector<LinuxNotificationState*> notifications;
    };
} // namespace PhotinoX::Native

#endif