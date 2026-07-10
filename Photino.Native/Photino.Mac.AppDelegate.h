#pragma once

#ifdef __APPLE__
#include <Cocoa/Cocoa.h>

@interface AppDelegate : NSObject <NSApplicationDelegate, NSUserNotificationCenterDelegate>
@end

#endif