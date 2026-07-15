#ifdef __APPLE__

#import <Cocoa/Cocoa.h>
#import <WebKit/WebKit.h>

#import <UserNotifications/UserNotifications.h>
#import "Photino.Mac.AppDelegate.h"
#import "Photino.Mac.NavigationDelegate.h"
#import "Photino.Mac.NSWindowBorderless.h"
#import "Photino.Mac.UiDelegate.h"
#import "Photino.Mac.WindowDelegate.h"

#include "Photino.h"
#include "Photino.Mac.State.h"
#include "Photino.Callbacks.h"
#include "Photino.Dialog.h"
#include "Photino.Strings.h"

#include <algorithm>
#include <atomic>
#include <cassert>
#include <memory>
#include <mutex>
#include <vector>

#include "Dependencies/json.hpp"

using json = nlohmann::json;
using namespace PhotinoX::Native;

namespace
{
    std::atomic_bool g_messageLoopRunning{ false };
    std::atomic_bool g_isShuttingDown{ false };
    Photino* g_messageLoopOwner = nullptr;
}

bool PhotinoMacIsShuttingDown()
{
    return g_isShuttingDown.load(std::memory_order_acquire);
}

void PhotinoMacSetShuttingDown(bool value)
{
    g_isShuttingDown.store(value, std::memory_order_release);
}

void PhotinoMacStopMessageLoopIfOwner(PhotinoX::Native::Photino* owner)
{
    if (owner != g_messageLoopOwner) return;

    g_isShuttingDown.store(true, std::memory_order_release);
    g_messageLoopOwner = nullptr;

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

//Creates an instance of the 'application' under which, all windows will run
//Only called once!
void Photino::Register()
{
    assert([NSThread isMainThread]);
    if (![NSThread isMainThread])
        std::abort();

    static std::once_flag flag;
    std::call_once(flag, [] {
        @autoreleasepool
        {
            static AppDelegate* appDelegate = nil;

            if (!appDelegate)
                appDelegate = [[AppDelegate alloc] init];

            NSApplication* application = [NSApplication sharedApplication];
            [application setDelegate:appDelegate];
            [application setActivationPolicy:NSApplicationActivationPolicyRegular];

            NSString* appName = [[NSProcessInfo processInfo] processName];

            NSMenu* mainMenu = [[[NSMenu alloc] init] autorelease];

            NSMenuItem* mainMenuItem = [[[NSMenuItem alloc] init] autorelease];
            [mainMenu addItem: mainMenuItem];

            NSMenu* mainSubMenu = [[[NSMenu alloc] init] autorelease];
            [mainMenuItem setSubmenu:mainSubMenu];

            // Add SelectAll, Cut, Copy & Paste Menu items to new edit menu
            // NSMenuItem *editMenuItem = [[
            //     [NSMenuItem alloc]
            //     initWithTitle: @"Edit"
            //     action: nil
            //     keyEquivalent: @""
            // ] autorelease];
            // [mainMenu addItem: editMenuItem];

            // NSMenu *editSubMenu = [[NSMenu new] autorelease];
            // [editMenuItem setSubmenu: editSubMenu];

            NSMenuItem* selectMenuItem = [[[NSMenuItem alloc]
                initWithTitle:@"Select All"
                action:@selector(selectAll:)
                keyEquivalent:@"a"] autorelease];
            // [editSubMenu addItem: selectMenuItem];
            [mainSubMenu addItem:selectMenuItem];

            NSMenuItem* cutMenuItem = [[[NSMenuItem alloc]
                initWithTitle:@"Cut"
                action:@selector(cut:)
                keyEquivalent:@"x"] autorelease];
            // [editSubMenu addItem: cutMenuItem];
            [mainSubMenu addItem:cutMenuItem];

            NSMenuItem* copyMenuItem = [[[NSMenuItem alloc]
                initWithTitle:@"Copy"
                action:@selector(copy:)
                keyEquivalent:@"c"] autorelease];
            // [editSubMenu addItem: copyMenuItem];
            [mainSubMenu addItem:copyMenuItem];

            NSMenuItem* pasteMenuItem = [[[NSMenuItem alloc]
                initWithTitle:@"Paste"
                action:@selector(paste:)
                keyEquivalent:@"v"] autorelease];
            // [editSubMenu addItem: pasteMenuItem];
            [mainSubMenu addItem:pasteMenuItem];

            // Add Quit Menu Item
            NSMenuItem* quitMenuItem = [[[NSMenuItem alloc]
                initWithTitle:[@"Quit " stringByAppendingString:appName]
                action:@selector(terminate:)
                keyEquivalent:@"q"] autorelease];
            [mainSubMenu addItem:quitMenuItem];

            [NSApp setMainMenu:mainMenu];

        }
    });
}

Photino::Photino(PhotinoInitParams* initParams) : platform_(std::make_unique<MacState>())
{
    @autoreleasepool
    {
        assert(initParams);
        if (!initParams)
            std::abort();

        if (initParams->Size != sizeof(PhotinoInitParams))
        {
            NSAlert* alert = [[[NSAlert alloc] init] autorelease];
            [alert setMessageText:@"Native Initialization Failed"];
            [alert setInformativeText:[NSString stringWithFormat:
                @"Initial parameters passed are %d bytes, but expected %zu bytes.",
                initParams->Size,
                sizeof(PhotinoInitParams)]];
            [alert runModal];

            std::abort();
        }

        _startString = ToPlatformString(initParams->StartString);
        _startUrl = ToPlatformString(initParams->StartUrl);
        _windowTitle = ToPlatformString(initParams->Title);
        _temporaryFilesPath = ToPlatformString(initParams->TemporaryFilesPath);
        _userAgent = ToPlatformString(initParams->UserAgent);
        _browserControlInitParameters = ToPlatformString(initParams->BrowserControlInitParameters);
        _notificationRegistrationId = ToPlatformString(initParams->NotificationRegistrationId);

        for (auto& customSchemeName : initParams->CustomSchemeNames)
        {
            AddCustomSchemeName(customSchemeName);
        }

        _parent = initParams->ParentInstance;

        //these handlers are ALWAYS hooked up
        _closingCallback = initParams->ClosingHandler;
        _focusInCallback = initParams->FocusInHandler;
        _focusOutCallback = initParams->FocusOutHandler;
        _resizedCallback = initParams->ResizedHandler;
        _maximizedCallback = initParams->MaximizedHandler;
        _restoredCallback = initParams->RestoredHandler;
        _minimizedCallback = initParams->MinimizedHandler;
        _movedCallback = initParams->MovedHandler;
        _webMessageReceivedCallback = initParams->WebMessageReceivedHandler;
        _customSchemeCallback = initParams->CustomSchemeHandler;
        _closedCallback = initParams->ClosedHandler;

        _zoom = initParams->Zoom;//??
        _chromeless = initParams->Chromeless;
        _fullScreen = initParams->FullScreen;
        _transparentEnabled = initParams->Transparent;// Set transparency (not yet implemented)
        _contextMenuEnabled = true; //not configurable on mac //initParams->ContextMenuEnabled;
        _zoomEnabled = initParams->ZoomEnabled;//??
         _devToolsEnabled = initParams->DevToolsEnabled;

         _grantBrowserPermissions = initParams->GrantBrowserPermissions;
        _fileSystemAccessEnabled = initParams->FileSystemAccessEnabled;
        _webSecurityEnabled = initParams->WebSecurityEnabled;
        _javascriptClipboardAccessEnabled = initParams->JavascriptClipboardAccessEnabled;
         _mediaStreamEnabled = initParams->MediaStreamEnabled;//??
        _ignoreCertificateErrorsEnabled = initParams->IgnoreCertificateErrorsEnabled;//??
        _notificationsEnabled = initParams->NotificationsEnabled;

        if (initParams->UseOsDefaultSize)
	    {
		    initParams->Width = 800; //CW_USEDEFAULT;
		    initParams->Height = 600; //CW_USEDEFAULT;
	    }
	    else
	    {
		    if (initParams->Width < 0) initParams->Width = 800; //CW_USEDEFAULT;
		    if (initParams->Height < 0) initParams->Height = 600; //CW_USEDEFAULT;
	    }

	    if (initParams->UseOsDefaultLocation)
	    {
		    initParams->Left = 0; //CW_USEDEFAULT;
		    initParams->Top = 0; //CW_USEDEFAULT;
	    }

        // Create Window
        NSRect frame = NSMakeRect(0, 0, 0, 0);

        if (initParams->Chromeless)
        {
            // For MouseMoved events, Photino.Mac.NSWindowBorderless.mm
            // https://stackoverflow.com/questions/2520127/getting-a-borderless-window-to-receive-mousemoved-events-cocoa-osx
            platform_->window = [[NSWindowBorderless alloc]
                initWithContentRect: frame
                styleMask: NSWindowStyleMaskBorderless
                    | NSWindowStyleMaskClosable
                    | NSWindowStyleMaskResizable
                    | NSWindowStyleMaskMiniaturizable
                backing: NSBackingStoreBuffered
                defer: true];
        }
        else
        {
            platform_->window = [[NSWindow alloc]
                initWithContentRect: frame
                styleMask: NSWindowStyleMaskTitled
                    | NSWindowStyleMaskClosable
                    | NSWindowStyleMaskResizable
                    | NSWindowStyleMaskMiniaturizable
                backing: NSBackingStoreBuffered
                defer: true];
        }

        if (!platform_->window)
            std::abort();

        [platform_->window setReleasedWhenClosed:NO];

        // Set Window Delegate
        platform_->windowDelegate = [[WindowDelegate alloc] init];
        if (!platform_->windowDelegate)
            std::abort();

        platform_->windowDelegate->photino = this;
        platform_->window.delegate = platform_->windowDelegate;
    
        // Set Window options
        SetTitle(_windowTitle);
        SetIconFile(ToPlatformString(initParams->WindowIconFile));

	    SetTopmost(initParams->Topmost);
        SetPosition(initParams->Left, initParams->Top);

        // It's important to set min/max size before setting size
        // SetSize is ensuring internally that the size is within min/max
        // but requires that min/max be set first.
        SetMinSize(initParams->MinWidth, initParams->MinHeight); // Defaults to 0,0
        SetMaxSize(initParams->MaxWidth, initParams->MaxHeight); // Defaults to 10000,10000
        SetSize(initParams->Width, initParams->Height);

	    SetMinimized(initParams->Minimized);
	    SetMaximized(initParams->Maximized);
    
	    SetResizable(initParams->Resizable);

	    if (initParams->CenterOnInitialize)
		    Photino::Center();
  
        // Create WebView Configuration
        platform_->webViewConfiguration = [[WKWebViewConfiguration alloc] init];
        if (!platform_->webViewConfiguration)
            std::abort();

        // Add Custom URL Schemes to WebView Configuration
        // Note that this can only be done *before* the WKWebView is instantiated, so we only let this
        // get called from the options callback in the constructor
        AddCustomSchemeHandlers();

        // Set initialized WebKit (Configuration) options
        SetPreference(@"developerExtrasEnabled", initParams->DevToolsEnabled ? @YES : @NO);
        SetPreference(@"allowFileAccessFromFileURLs", initParams->FileSystemAccessEnabled ? @YES : @NO);
        SetPreference(@"webSecurityEnabled", initParams->WebSecurityEnabled ? @YES : @NO);
        SetPreference(@"javaScriptCanAccessClipboard", initParams->JavascriptClipboardAccessEnabled ? @YES : @NO);
        SetPreference(@"mediaStreamEnabled", initParams->MediaStreamEnabled ? @YES : @NO);

        SetPreference(@"mediaDevicesEnabled", @YES);
        SetPreference(@"mediaCaptureRequiresSecureConnection", @NO);

        if ([NSProcessInfo.processInfo isOperatingSystemAtLeastVersion: NSOperatingSystemVersion({13, 3, 0})])
        {
            SetPreference(@"notificationEventEnabled", @YES);
        }

        SetPreference(@"notificationsEnabled", @YES);
        SetPreference(@"screenCaptureEnabled", @YES);

        if (!_browserControlInitParameters.empty())
        {
            // Set initialized WebKit (Configuration) options
            json wkPreferences = json::parse(_browserControlInitParameters, nullptr, false);
            if (wkPreferences.is_discarded() || !wkPreferences.is_object())
                std::abort();

            // Iterate over wkPreferences json object and set preferences
            for (json::iterator it = wkPreferences.begin(); it != wkPreferences.end(); ++it)
            {
                std::string key = it.key();
                json value = it.value();
            
                NSString* preferenceKey = [NSString stringWithUTF8String:key.c_str()];
                if (!preferenceKey) continue;

                if (value.is_number_integer())
                {
                    SetPreference(preferenceKey, [NSNumber numberWithInt:value.get<int>()]);
                }
                else if (value.is_number_float())
                {
                    SetPreference(preferenceKey, [NSNumber numberWithDouble:value.get<double>()]);
                }
                else if (value.is_boolean())
                {
                    SetPreference(preferenceKey, [NSNumber numberWithBool:value.get<bool>()]);
                }
                else if (value.is_string())
                {
                    std::string stringValue = value.get<std::string>();
                    NSString* preferenceValue = [[NSString alloc] initWithUTF8String:stringValue.c_str()];
                    if (preferenceValue)
                    {
                        SetPreference(preferenceKey, preferenceValue);
                        [preferenceValue release];
                    }
                }
            }
        }

        // Create WebView
        AttachWebView();

        _dialog = new PhotinoDialog();

        Show();
        SetFullScreen(initParams->FullScreen);
    }
}

Photino::~Photino()
{
    if (platform_->webViewConfiguration)
    {
        WKUserContentController* userContentController = platform_->webViewConfiguration.userContentController;
        if (userContentController)
            [userContentController removeScriptMessageHandlerForName:@"photinointerop"];
    }

    if (platform_->window)
        platform_->window.delegate = nil;

    if (platform_->windowDelegate)
    {
        platform_->windowDelegate->photino = nullptr;
        [platform_->windowDelegate release];
        platform_->windowDelegate = nil;
    }

    if (platform_->webView)
    {
        platform_->webView.UIDelegate = nil;
        platform_->webView.navigationDelegate = nil;
    }

    if (platform_->uiDelegate)
    {
        platform_->uiDelegate->photino = nullptr;
        platform_->uiDelegate->window = nil;
        platform_->uiDelegate->webMessageReceivedCallback = nullptr;
        [platform_->uiDelegate release];
        platform_->uiDelegate = nil;
    }

    if (platform_->navigationDelegate)
    {
        platform_->navigationDelegate->photino = nullptr;
        platform_->navigationDelegate->window = nil;
        [platform_->navigationDelegate release];
        platform_->navigationDelegate = nil;
    }

    [platform_->webView release];
    platform_->webView = nil;

    [platform_->webViewConfiguration release];
    platform_->webViewConfiguration = nil;

    //[platform_->window performClose: platform_->window];
    [platform_->window release];
    platform_->window = nil;

    delete _dialog;
    _dialog = nullptr;
    //[NSApp release];
}

void Photino::GetNotificationsEnabled(bool* enabled) const
{
    assert(enabled);
    if (!enabled) return;

    *enabled = _notificationsEnabled;
}

void Photino::ShowNotification(const PlatformString& title, const PlatformString& body) const
{
    if (!_notificationsEnabled) return;

    NSString* nsTitle = ToNSString(title);
    if (!nsTitle) return;

    NSString* nsBody = ToNSString(body);
    if (!nsBody) return;

    UNMutableNotificationContent* notificationContent = [[[UNMutableNotificationContent alloc] init] autorelease];
    notificationContent.title = nsTitle;
    notificationContent.body = nsBody;
    notificationContent.sound = [UNNotificationSound defaultSound];

    UNTimeIntervalNotificationTrigger* trigger =
        [UNTimeIntervalNotificationTrigger triggerWithTimeInterval:0.3 repeats:NO];

    NSString* identifier = [[NSUUID UUID] UUIDString];

    UNNotificationRequest* request =
        [UNNotificationRequest requestWithIdentifier:identifier
                                             content:notificationContent
                                             trigger:trigger];

    UNUserNotificationCenter* center = [UNUserNotificationCenter currentNotificationCenter];

    [center addNotificationRequest:request withCompletionHandler:^(NSError* error) {
        if (error)
            NSLog(@"Failed to show notification: %@", error);
    }];
}

void Photino::WaitForExit() const
{
    bool expected = false;
    if (!g_messageLoopRunning.compare_exchange_strong(expected, true, std::memory_order_acq_rel)) return;

    g_isShuttingDown.store(false, std::memory_order_release);
    g_messageLoopOwner = const_cast<Photino*>(this);

    [NSApp run];

    g_messageLoopOwner = nullptr;
    g_isShuttingDown.store(true, std::memory_order_release);
    g_messageLoopRunning.store(false, std::memory_order_release);
}

//Callbacks

void Photino::Invoke(InvokeCallback callback) const
{
    assert(callback);
    if (!callback) return;

    if (g_isShuttingDown.load(std::memory_order_acquire)) return;

    if ([NSThread isMainThread])
    {
        callback();
        return;
    }

    if (!g_messageLoopRunning.load(std::memory_order_acquire)) return;

    dispatch_sync(dispatch_get_main_queue(), ^{
        callback();
    });
}

void Photino::Show()
{
    if (platform_->webView == nil)
        AttachWebView();

    [platform_->window makeKeyAndOrderFront:platform_->window];
    [platform_->window orderFrontRegardless];
}

#endif