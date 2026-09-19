#pragma once

#ifdef __APPLE__

#import <UserNotifications/UserNotifications.h>

namespace PhotinoX::Native
{
    class PhotinoApplication;
}

@interface NotificationDelegate : NSObject <UNUserNotificationCenterDelegate>
{
  @public
    PhotinoX::Native::PhotinoApplication* app;
}
@end

#endif