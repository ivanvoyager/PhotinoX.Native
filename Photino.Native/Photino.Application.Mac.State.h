#pragma once

#ifdef __APPLE__

@class AppDelegate;
@class NotificationDelegate;

namespace PhotinoX::Native
{
    struct MacApplicationState
    {
        AppDelegate* appDelegate = nullptr;
        NotificationDelegate* notificationDelegate = nullptr;
    };
} // namespace PhotinoX::Native

#endif