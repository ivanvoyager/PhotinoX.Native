#ifdef __APPLE__

#import "Photino.Application.Mac.NotificationDelegate.h"

#include "Photino.Application.h"
#include "Photino.Application.NotificationDispatch.h"
#include "Photino.Strings.h"

#include <cstdint>
#include <memory>
#include <string>

using namespace PhotinoX::Native;

namespace
{
    void ScheduleNotificationInputActivated(const PhotinoApplication* app, int notificationId, NSString* response, void* callbackState)
    {
        const char* utf8 = response ? [response UTF8String] : nullptr;

        NotificationDispatch::ScheduleNotificationInputActivated(app, notificationId, utf8 ? std::string(utf8) : std::string(), callbackState);
    }

    int GetNotificationId(UNNotificationResponse* response)
    {
        NSNumber* value = response.notification.request.content.userInfo[@"notificationId"];
        return value ? [value intValue] : 0;
    }

    void* GetCallbackState(UNNotificationResponse* response)
    {
        NSNumber* value = response.notification.request.content.userInfo[@"callbackState"];
        if (!value)
            return nullptr;

        return reinterpret_cast<void*>(static_cast<uintptr_t>([value unsignedLongLongValue]));
    }

    bool TryGetActionIndex(NSString* actionIdentifier, int* actionIndex)
    {
        if (!actionIdentifier || !actionIndex)
            return false;

        NSCharacterSet* nonDigits = [[NSCharacterSet decimalDigitCharacterSet] invertedSet];
        if ([actionIdentifier rangeOfCharacterFromSet:nonDigits].location != NSNotFound)
            return false;

        *actionIndex = [actionIdentifier intValue];
        return true;
    }
}

@implementation NotificationDelegate

- (void)userNotificationCenter:(UNUserNotificationCenter*)center
       willPresentNotification:(UNNotification*)notification
         withCompletionHandler:(void (^)(UNNotificationPresentationOptions options))completionHandler
{
    //NSLog(@"PhotinoX: macOS notification will present: %@", notification.request.identifier);

    completionHandler(UNNotificationPresentationOptionBanner |
                      UNNotificationPresentationOptionList |
                      UNNotificationPresentationOptionSound);
}

- (void)userNotificationCenter:(UNUserNotificationCenter*)center
didReceiveNotificationResponse:(UNNotificationResponse*)response
         withCompletionHandler:(void (^)(void))completionHandler
{
    if (!app)
    {
        completionHandler();
        return;
    }

    int notificationId = GetNotificationId(response);
    void* callbackState = GetCallbackState(response);

    if ([response.actionIdentifier isEqualToString:UNNotificationDismissActionIdentifier])
    {
        NotificationDispatch::ScheduleNotificationDismissed(app, notificationId, PhotinoNotificationDismissalReason::UserCanceled, callbackState);
        completionHandler();
        return;
    }

    if ([response isKindOfClass:[UNTextInputNotificationResponse class]])
    {
        UNTextInputNotificationResponse* inputResponse = static_cast<UNTextInputNotificationResponse*>(response);
        ScheduleNotificationInputActivated(app, notificationId, inputResponse.userText, callbackState);
        completionHandler();
        return;
    }

    if (![response.actionIdentifier isEqualToString:UNNotificationDefaultActionIdentifier])
    {
        int actionIndex = -1;
        if (TryGetActionIndex(response.actionIdentifier, &actionIndex))
        {
            NotificationDispatch::ScheduleNotificationActionActivated(app, notificationId, actionIndex, callbackState);
            completionHandler();
            return;
        }
    }

    NotificationDispatch::ScheduleNotificationActivated(app, notificationId, callbackState);
    completionHandler();
}

@end
#endif