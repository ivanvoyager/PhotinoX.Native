#include "Photino.Application.h"
#include "Photino.Application.InitParams.h"
#include "Photino.Application.NotificationDispatch.h"
#include "Photino.Application.Mac.State.h"
#include "Photino.Strings.h"

#import <UserNotifications/UserNotifications.h>
#import <Cocoa/Cocoa.h>

#import "Photino.Application.Mac.NotificationDelegate.h"

#include <cassert>
#include <cstdint>
#include <cstdlib>

using namespace PhotinoX::Native;

namespace
{
    void StopApplicationLoop()
    {
        [NSApp stop:nil];

        NSEvent* event = [NSEvent otherEventWithType:NSEventTypeApplicationDefined
                                            location:NSZeroPoint
                                       modifierFlags:0
                                           timestamp:0
                                        windowNumber:0
                                             context:nil
                                             subtype:0
                                               data1:0
                                               data2:0];

        [NSApp postEvent:event atStart:NO];
    }

    NSString* const PhotinoNotificationCategoryIdentifier = @"PhotinoX.Notification";
}

PhotinoApplication::PhotinoApplication() : platform_(std::make_unique<MacApplicationState>())
{
}

PhotinoApplication::~PhotinoApplication()
{
    if (platform_->notificationDelegate)
    {
        UNUserNotificationCenter* center = [UNUserNotificationCenter currentNotificationCenter];

        if (center.delegate == platform_->notificationDelegate)
            center.delegate = nil;

        platform_->notificationDelegate->app = nullptr;
        [platform_->notificationDelegate release];
        platform_->notificationDelegate = nil;
    }
}

void PhotinoApplication::ValidateInitParams(const PhotinoApplicationInitParams* initParams)
{
    if (!initParams)
        std::abort();

    if (initParams->Size != sizeof(PhotinoApplicationInitParams) ||
        initParams->AbiVersion != PhotinoApplicationInitParams::NativeAbiVersion)
    {
        NSAlert* alert = [[[NSAlert alloc] init] autorelease];

        [alert setMessageText:@"Native Initialization Failed"];
        [alert setInformativeText:[NSString stringWithFormat:
            @"Application initial parameters ABI mismatch. Passed size: %d bytes, expected size: %zu bytes. Passed ABI version: %d, expected ABI version: %d.",
            initParams->Size,
            sizeof(PhotinoApplicationInitParams),
            initParams->AbiVersion,
            PhotinoApplicationInitParams::NativeAbiVersion]];

        [alert runModal];

        std::abort();
    }
}

int PhotinoApplication::RunCore()
{
    assert([NSThread isMainThread]);
    if (![NSThread isMainThread])
        return -1;

    @autoreleasepool
    {
        [NSApp run];
        return exitCode_.load(std::memory_order_acquire);
    }
}

void PhotinoApplication::ShutdownCore(int exitCode) noexcept
{
    exitCode_.store(exitCode, std::memory_order_release);

    if ([NSThread isMainThread])
    {
        StopApplicationLoop();
        return;
    }

    dispatch_async(dispatch_get_main_queue(), ^{
        StopApplicationLoop();
    });
}

bool PhotinoApplication::CheckAccess() const noexcept
{
    return [NSThread isMainThread];
}

bool PhotinoApplication::Invoke(InvokeStateCallback callback, void* state) const
{
    assert(callback);

    if (!callback || IsShuttingDown())
        return false;

    if (CheckAccess())
    {
        callback(state);
        return true;
    }

    if (!IsRunning())
        return false;

    dispatch_sync(dispatch_get_main_queue(), ^{
        callback(state);
    });

    return true;
}

bool PhotinoApplication::BeginInvoke(InvokeStateCallback callback, void* state) const
{
    assert(callback);

    if (!callback || IsShuttingDown() || !IsRunning())
        return false;

    dispatch_async(dispatch_get_main_queue(), ^{
        callback(state);
    });

    return true;
}

bool PhotinoApplication::IsAppBundleProcess() const
{
    NSBundle* mainBundle = [NSBundle mainBundle];
    if (!mainBundle)
        return false;

    NSURL* bundleURL = [mainBundle bundleURL];
    if (!bundleURL)
        return false;

    NSString* extension = [bundleURL pathExtension];
    return extension && [extension caseInsensitiveCompare:@"app"] == NSOrderedSame;
}

bool PhotinoApplication::InitializeNotifications()
{
    bool expected = false;
    if (!notificationsInitialized_.compare_exchange_strong(expected, true, std::memory_order_acq_rel))
        return true;

    //NSBundle* mainBundle = [NSBundle mainBundle];
    //NSLog(@"PhotinoX: macOS notification bundle id: %@", [mainBundle bundleIdentifier]);
    //NSLog(@"PhotinoX: macOS notification bundle url: %@", [mainBundle bundleURL]);

    if (!IsAppBundleProcess())
    {
        NSLog(@"PhotinoX: macOS native notifications require an .app bundle. Current process is not running from an application bundle.");
        notificationsInitialized_.store(false, std::memory_order_release);
        return false;
    }

    UNUserNotificationCenter* center = nil;

    @try
    {
        center = [UNUserNotificationCenter currentNotificationCenter];
    }
    @catch (NSException* exception)
    {
        NSLog(@"PhotinoX: Failed to initialize macOS native notifications: %@", exception);
        notificationsInitialized_.store(false, std::memory_order_release);
        return false;
    }

    if (!center)
    {
        NSLog(@"PhotinoX: Failed to initialize macOS native notifications: current notification center is nil.");
        notificationsInitialized_.store(false, std::memory_order_release);
        return false;
    }

    if (!platform_->notificationDelegate)
        platform_->notificationDelegate = [[NotificationDelegate alloc] init];

    platform_->notificationDelegate->app = this;
    center.delegate = platform_->notificationDelegate;

    UNNotificationCategory* category =
        [UNNotificationCategory categoryWithIdentifier:PhotinoNotificationCategoryIdentifier
                                               actions:@[]
                                     intentIdentifiers:@[]
                                               options:UNNotificationCategoryOptionCustomDismissAction];

    [center setNotificationCategories:[NSSet setWithObject:category]];

    [center requestAuthorizationWithOptions:(UNAuthorizationOptionAlert |
                                             UNAuthorizationOptionSound |
                                             UNAuthorizationOptionBadge)
                          completionHandler:^(BOOL granted, NSError* error) {
                              if (error)
                              {
                                  NSLog(@"PhotinoX: macOS notification authorization request failed: %@", error);
                                  return;
                              }

                              if (!granted)
                              {
                                  NSLog(@"PhotinoX: macOS notification authorization was not granted.");
                                  return;
                              }

                              /*NSLog(@"PhotinoX: macOS notification authorization granted: %@", granted ? @"YES" : @"NO");

                              [[UNUserNotificationCenter currentNotificationCenter]
                                  getNotificationSettingsWithCompletionHandler:^(UNNotificationSettings* settings) {
                                      NSLog(@"PhotinoX: macOS notification settings: authorizationStatus=%ld alertSetting=%ld soundSetting=%ld notificationCenterSetting=%ld",
                                            (long)settings.authorizationStatus,
                                            (long)settings.alertSetting,
                                            (long)settings.soundSetting,
                                            (long)settings.notificationCenterSetting);
                                  }];*/
                          }];

    return true;
}

void PhotinoApplication::UninitializeNotifications() noexcept
{
    if (!notificationsInitialized_.exchange(false, std::memory_order_acq_rel))
        return;

    if (!platform_->notificationDelegate)
        return;

    UNUserNotificationCenter* center = [UNUserNotificationCenter currentNotificationCenter];

    if (center.delegate == platform_->notificationDelegate)
        center.delegate = nil;

    platform_->notificationDelegate->app = nullptr;
}

int PhotinoApplication::ShowNotificationCore(int notificationId, const PlatformString& title, const PlatformString& body, const PlatformString& iconPath, void* callbackState)
{
    NSString* nsTitle = ToNSString(title);
    if (!nsTitle)
        return -1;

    NSString* nsBody = ToNSString(body);
    if (!nsBody)
        return -1;

    UNMutableNotificationContent* content = [[[UNMutableNotificationContent alloc] init] autorelease];
    content.title = nsTitle;
    content.body = nsBody;
    content.sound = [UNNotificationSound defaultSound];
    content.categoryIdentifier = PhotinoNotificationCategoryIdentifier;

    if (@available(macOS 12.0, *))
    {
        content.interruptionLevel = UNNotificationInterruptionLevelActive;
    }

    content.userInfo =
    @{
        @"notificationId": @(notificationId),
        @"callbackState": @((unsigned long long)(uintptr_t)callbackState)
    };

    if (!iconPath.empty())
    {
        NSString* nsImagePath = ToNSString(iconPath);
        if (nsImagePath)
        {
            NSURL* imageUrl = [NSURL fileURLWithPath:nsImagePath];
            NSError* attachmentError = nil;

            UNNotificationAttachment* attachment =
                [UNNotificationAttachment attachmentWithIdentifier:@"icon"
                                                               URL:imageUrl
                                                           options:nil
                                                             error:&attachmentError];

            if (attachment)
                content.attachments = @[attachment];
        }
    }

    NSString* identifier = [NSString stringWithFormat:@"%d-%@", notificationId, [[NSUUID UUID] UUIDString]];

    UNNotificationRequest* request =
        [UNNotificationRequest requestWithIdentifier:identifier
                                             content:content
                                             trigger:nil];

    const PhotinoApplication* app = this;

    UNUserNotificationCenter* center = [UNUserNotificationCenter currentNotificationCenter];

    [center getNotificationSettingsWithCompletionHandler:^(UNNotificationSettings* settings) {
        if (!settings || settings.authorizationStatus == UNAuthorizationStatusDenied)
        {
            NotificationDispatch::ScheduleNotificationFailed(app, notificationId, callbackState);
            return;
        }

        [center addNotificationRequest:request
                 withCompletionHandler:^(NSError* error) {
                     if (error)
                     {
                         NSLog(@"PhotinoX: Failed to show macOS notification: %@", error);
                         NotificationDispatch::ScheduleNotificationFailed(app, notificationId, callbackState);
                         return;
                     }

                     //NSLog(@"PhotinoX: macOS notification request added: notificationId=%d requestId=%@", notificationId, identifier);
                 }];
    }];

    return notificationId;
}