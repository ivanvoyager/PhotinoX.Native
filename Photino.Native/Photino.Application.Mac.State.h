#pragma once

#ifdef __APPLE__

@class NotificationDelegate;

namespace PhotinoX::Native
{
    struct MacApplicationState
    {
        NotificationDelegate* notificationDelegate = nullptr;
    };
} // namespace PhotinoX::Native

#endif