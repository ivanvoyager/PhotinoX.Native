//#define __APPLE__ 1
#ifdef __APPLE__
#include "Photino.h"
#include "Photino.Dialog.h"
#include "Photino.Strings.h"
#include "Photino.Mac.AppDelegate.h"
#include "Photino.Mac.UiDelegate.h"
#include "Photino.Mac.WindowDelegate.h"
#include "Photino.Mac.UrlSchemeHandler.h"
#include "Photino.Mac.NSWindowBorderless.h"
#include "Photino.Mac.NavigationDelegate.h"
#include "Photino.Mac.State.h"

#include <algorithm>
#include <atomic>
#include <cassert>
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

Photino::Photino(PhotinoInitParams* initParams)
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
            _window = [[NSWindowBorderless alloc]
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
            _window = [[NSWindow alloc]
                initWithContentRect: frame
                styleMask: NSWindowStyleMaskTitled
                    | NSWindowStyleMaskClosable
                    | NSWindowStyleMaskResizable
                    | NSWindowStyleMaskMiniaturizable
                backing: NSBackingStoreBuffered
                defer: true];
        }

        if (!_window)
            std::abort();

        [_window setReleasedWhenClosed:NO];

        // Set Window Delegate
        _windowDelegate = [[WindowDelegate alloc] init];
        if (!_windowDelegate)
            std::abort();

        _windowDelegate->photino = this;
        _window.delegate = _windowDelegate;
    
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
        _webviewConfiguration = [[WKWebViewConfiguration alloc] init];
        if (!_webviewConfiguration)
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
    if (_webviewConfiguration)
    {
        WKUserContentController* userContentController = _webviewConfiguration.userContentController;
        if (userContentController)
            [userContentController removeScriptMessageHandlerForName:@"photinointerop"];
    }

    if (_window)
        _window.delegate = nil;

    if (_windowDelegate)
    {
        _windowDelegate->photino = nullptr;
        [_windowDelegate release];
        _windowDelegate = nil;
    }

    if (_webview)
    {
        _webview.UIDelegate = nil;
        _webview.navigationDelegate = nil;
    }

    if (_uiDelegate)
    {
        _uiDelegate->photino = nullptr;
        _uiDelegate->window = nil;
        _uiDelegate->webMessageReceivedCallback = nullptr;
        [_uiDelegate release];
        _uiDelegate = nil;
    }

    if (_navigationDelegate)
    {
        _navigationDelegate->photino = nullptr;
        _navigationDelegate->window = nil;
        [_navigationDelegate release];
        _navigationDelegate = nil;
    }

    [_webview release];
    _webview = nil;

    [_webviewConfiguration release];
    _webviewConfiguration = nil;

    //[_window performClose: _window];
    [_window release];
    _window = nil;

    delete _dialog;
    _dialog = nullptr;
    //[NSApp release];
}

void Photino::Center()
{
    assert(_window);
    if (!_window) return;

    [_window center];
    //[_window makeKeyAndOrderFront:_window];

    //NSRect screen = [[_window screen] visibleFrame];
    //NSRect window = [_window frame];
    //CGFloat xPos = NSWidth(screen) / 2 + screen.origin.x - NSWidth(window) / 2;
    //CGFloat yPos = NSHeight(screen) / 2 + screen.origin.y - NSHeight(window) / 2;
    //[_window setFrame: NSMakeRect(xPos, yPos, NSWidth(window), NSHeight(window)) display:YES];
}

void Photino::ClearBrowserAutoFill() const
{
    //TODO
}



void Photino::GetTransparentEnabled(bool* enabled) const
{
    assert(enabled);
    if (!enabled) return;
    //! Not implemented (supported?) on macOS
    *enabled = _transparentEnabled;
}

void Photino::GetContextMenuEnabled(bool* enabled) const
{
    assert(enabled);
    if (!enabled) return;

    *enabled = _contextMenuEnabled;
}

void Photino::GetZoomEnabled(bool* enabled) const
{
    assert(enabled);
    if (!enabled) return;

    *enabled = _zoomEnabled;
}

void Photino::GetDevToolsEnabled(bool* enabled) const
{
    assert(enabled);
    if (!enabled) return;

    *enabled = _devToolsEnabled;
}

void Photino::GetGrantBrowserPermissions(bool* enabled) const
{
    assert(enabled);
    if (!enabled) return;

    *enabled = _grantBrowserPermissions;
}

//! Always enabled on macOS. This is always true.
void Photino::GetMediaAutoplayEnabled(bool* enabled) const
{
    assert(enabled);
    if (!enabled) return;

    *enabled = true;
}

void Photino::GetFileSystemAccessEnabled(bool* enabled) const
{
    assert(enabled);
    if (!enabled) return;

    *enabled = _fileSystemAccessEnabled;
}

//! Not supported on macOS. This is always false.
void Photino::GetSmoothScrollingEnabled(bool* enabled) const
{
    assert(enabled);
    if (!enabled) return;

    *enabled = false;
}

void Photino::GetWebSecurityEnabled(bool* enabled) const
{
    assert(enabled);
    if (!enabled) return;

    *enabled = _webSecurityEnabled;
}

void Photino::GetJavascriptClipboardAccessEnabled(bool* enabled) const
{
    assert(enabled);
    if (!enabled) return;

    *enabled = _javascriptClipboardAccessEnabled;
}

void Photino::GetMediaStreamEnabled(bool* enabled) const
{
    assert(enabled);
    if (!enabled) return;

    *enabled = _mediaStreamEnabled;
}

void Photino::GetFullScreen(bool* fullScreen) const
{
    assert(fullScreen);
    if (!fullScreen) return;

    *fullScreen = false;

    if (!_window) return;

    //*fullScreen = ([_window.contentView isInFullScreenMode]);
    *fullScreen = ([_window styleMask] & NSWindowStyleMaskFullScreen) == NSWindowStyleMaskFullScreen;
}

void Photino::GetMaximized(bool* isMaximized) const
{
    assert(isMaximized);
    if (!isMaximized) return;

    *isMaximized = false;

    if (!_window) return;

    *isMaximized = [_window isZoomed];
}

void Photino::GetMinimized(bool* isMinimized) const
{
    assert(isMinimized);
    if (!isMinimized) return;

    *isMinimized = false;

    if (!_window) return;

	*isMinimized = [_window isMiniaturized];
}

void Photino::GetResizable(bool* resizable) const
{
    assert(resizable);
    if (!resizable) return;

    *resizable = false;

    if (!_window) return;

    *resizable = ([_window styleMask] & NSWindowStyleMaskResizable) == NSWindowStyleMaskResizable;
}

void Photino::GetIgnoreCertificateErrorsEnabled(bool* enabled) const
{
    assert(enabled);
    if (!enabled) return;

	*enabled = _ignoreCertificateErrorsEnabled;
}

void Photino::GetNotificationsEnabled(bool* enabled) const
{
    assert(enabled);
    if (!enabled) return;

    *enabled = _notificationsEnabled;
}

unsigned int Photino::GetScreenDpi() const
{
    //not supported on macOS - _window's devices collection does have dpi
	//https://stackoverflow.com/questions/2621439/hot-to-get-screen-dpi-linux-mac-programaticaly
    if (!_window) return 72;

    NSScreen* screen = [_window screen];
    if (!screen) return 72;

    return static_cast<unsigned int>(roundf(72.0f * [screen backingScaleFactor]));
}

/*AutoString Photino::GetTitle() const
{
    return _windowTitle;
}*/

void Photino::GetTopmost(bool* topmost) const
{
    assert(topmost);
    if (!topmost) return;

    *topmost = false;

    if (!_window) return;

    *topmost = [_window level] == NSFloatingWindowLevel;
}

void Photino::GetZoom(int* zoom) const
{
    assert(zoom);
    if (!zoom) return;

    *zoom = _zoom;

    if (!_webview) return;

    CGFloat rawValue = [_webview magnification];
    rawValue = (rawValue * 100.0) + 0.5;
    *zoom = static_cast<int>(rawValue);
}

void Photino::NavigateToString(const PlatformString& content) const
{
    assert(_webview);
    if (!_webview) return;

    NSString* nsContent = ToNSString(content);
    if (!nsContent) return;

    [_webview loadHTMLString:nsContent baseURL:nil];
}

void Photino::NavigateToUrl(const PlatformString& url) const
{
    assert(_webview);
    if (!_webview || url.empty()) return;

    NSString* nsUrlString = ToNSString(url);
    if (!nsUrlString) return;

    NSURL* nsUrl = [NSURL URLWithString:nsUrlString];
    if (!nsUrl) return;

    NSURLRequest* nsRequest = [NSURLRequest requestWithURL:nsUrl];
    if (!nsRequest) return;

    [_webview loadRequest:nsRequest];
}

void Photino::Restore() const
{
    assert(_window);
    if (!_window) return;

    if ([_window isMiniaturized])
        [_window deminiaturize:nil];

    if ([_window isZoomed])
        [_window zoom:nil];

    if (([_window styleMask] & NSWindowStyleMaskFullScreen) == NSWindowStyleMaskFullScreen)
        [_window toggleFullScreen:nil];
}

void Photino::SendWebMessage(const PlatformString& message) const
{
    assert(_webview);
    if (!_webview) return;

    PlatformString script;
    script.append("__dispatchMessageCallback(");
    script.append(json(message).dump(-1, ' ', false, json::error_handler_t::replace));
    script.append(")");

    NSString* javaScriptToEval = ToNSString(script);
    if (!javaScriptToEval) return;

    [_webview evaluateJavaScript:javaScriptToEval completionHandler:nil];
}

void Photino::SetUserAgent(const PlatformString& userAgent)
{
    _userAgent = userAgent;

    if (!_webview) return;

    NSString* nsUserAgent = ToNSString(userAgent);
    if (!nsUserAgent) return;

    [_webview setCustomUserAgent:nsUserAgent];
}

// Set preferences with a string key and a value of any type
bool Photino::SetPreference(NSString* key, NSNumber* value)
{
    assert(_webviewConfiguration && key && value);
    if (!_webviewConfiguration || !key || !value) return false;

    @try
    {
        [_webviewConfiguration.preferences setValue:value forKey:key];
        return true;
    }
    @catch (NSException* exception)
    {
        return false;
    }
}

bool Photino::SetPreference(NSString* key, NSString* value)
{
    assert(_webviewConfiguration && key && value);
    if (!_webviewConfiguration || !key || !value) return false;

    @try
    {
        [_webviewConfiguration.preferences setValue:value forKey:key];
        return true;
    }
    @catch (NSException* exception)
    {
        return false;
    }
}

void Photino::SetDevToolsEnabled(bool enabled)
{
    _devToolsEnabled = enabled;

    SetPreference(@"developerExtrasEnabled", enabled ? @YES : @NO);
}

void Photino::SetTransparentEnabled(bool enabled)
{
    _transparentEnabled = enabled;

    //! Not implemented (supported?) on macOS
}

void Photino::SetContextMenuEnabled(bool enabled)
{
    _contextMenuEnabled = enabled;

    //! Not supported on macOS
}

void Photino::SetZoomEnabled(bool enabled)
{
    _zoomEnabled = enabled;

    //! Not implemented (supported?) on macOS
}

void Photino::SetFullScreen(bool fullScreen)
{
    assert(_window);
    if (!_window) return;

    bool isFullScreen = ([_window styleMask] & NSWindowStyleMaskFullScreen) == NSWindowStyleMaskFullScreen;
    if (isFullScreen == fullScreen) return;

    _fullScreen = fullScreen;
    [_window toggleFullScreen:nil];
}

void Photino::SetMinimized(bool minimized)
{
    assert(_window);
    if (!_window) return;

    if (_window.isMiniaturized == minimized) return;

    if (minimized)
        [_window miniaturize:nil];
    else
        [_window deminiaturize:nil];
}

void Photino::SetMaximized(bool maximized)
{
    assert(_window);
    if (!_window) return;

    if ([_window isZoomed] == maximized) return;

    [_window zoom:nil];
}

void Photino::SetResizable(bool resizable)
{
    assert(_window);
    if (!_window) return;

    NSWindowStyleMask styleMask = [_window styleMask];

    if (resizable)
        styleMask |= NSWindowStyleMaskResizable;
    else
        styleMask &= ~NSWindowStyleMaskResizable;

    [_window setStyleMask:styleMask];
}

void Photino::SetTopmost(bool topmost)
{
    assert(_window);
    if (!_window) return;

    [_window setLevel:topmost ? NSFloatingWindowLevel : NSNormalWindowLevel];
}

void Photino::SetZoom(int zoom)
{
    if (zoom < 25)
        zoom = 25;
    else if (zoom > 500)
        zoom = 500;

    _zoom = zoom;

    if (!_webview) return;

    CGFloat newZoom = static_cast<CGFloat>(zoom) / 100.0;
    [_webview setMagnification:newZoom];
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
void Photino::GetAllMonitors(GetAllMonitorsCallback callback) const noexcept
{
    assert(callback);
    if (!callback) return;

    for (NSScreen* screen in [NSScreen screens])
    {
        Monitor props{};

        NSRect frame = [screen frame];
        props.monitor.x = static_cast<int>(roundf(frame.origin.x));
        props.monitor.y = static_cast<int>(roundf(frame.origin.y));
        props.monitor.width = static_cast<int>(roundf(frame.size.width));
        props.monitor.height = static_cast<int>(roundf(frame.size.height));

        NSRect visibleFrame = [screen visibleFrame];
        props.work.x = static_cast<int>(roundf(visibleFrame.origin.x));
        props.work.y = static_cast<int>(roundf(visibleFrame.origin.y));
        props.work.width = static_cast<int>(roundf(visibleFrame.size.width));
        props.work.height = static_cast<int>(roundf(visibleFrame.size.height));

        props.scale = [screen backingScaleFactor];

        if (!callback(&props))
            break;
    }
}

std::vector<Monitor> Photino::GetMonitors() const
{
    std::vector<Monitor> monitors;

    for (NSScreen* screen in [NSScreen screens])
    {
        NSRect monitorFrame = [screen frame];
        NSRect workFrame = [screen visibleFrame];

        Monitor monitor{};
        monitor.monitor.x = static_cast<int>(roundf(monitorFrame.origin.x));
        monitor.monitor.y = static_cast<int>(roundf(monitorFrame.origin.y));
        monitor.monitor.width = static_cast<int>(roundf(monitorFrame.size.width));
        monitor.monitor.height = static_cast<int>(roundf(monitorFrame.size.height));

        monitor.work.x = static_cast<int>(roundf(workFrame.origin.x));
        monitor.work.y = static_cast<int>(roundf(workFrame.origin.y));
        monitor.work.width = static_cast<int>(roundf(workFrame.size.width));
        monitor.work.height = static_cast<int>(roundf(workFrame.size.height));

        monitor.scale = [screen backingScaleFactor];

        monitors.push_back(monitor);
    }

    return monitors;
}

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

//private methods

void Photino::AddCustomSchemeHandlers()
{
    assert(!_webview);
    if (_webview) return;

    assert(_webviewConfiguration);
    if (!_webviewConfiguration || !_customSchemeCallback) return;

    for (const auto& scheme : _customSchemeNames)
    {
        NSString* nsScheme = ToNSString(scheme);
        if (!nsScheme) continue;

        UrlSchemeHandler* schemeHandler = [[UrlSchemeHandler alloc] init];
        if (!schemeHandler) continue;

        schemeHandler->requestHandler = _customSchemeCallback;

        @try
        {
            [_webviewConfiguration setURLSchemeHandler:schemeHandler forURLScheme:nsScheme];
        }
        @catch (NSException* exception)
        {
            [schemeHandler release];
            continue;
        }

        [schemeHandler release];
    }
}

bool Photino::RegisterCustomSchemeName(const PlatformString& scheme)
{
    if (!_webviewConfiguration) return true;

    if (_webview) return false;

    return true;
}

void Photino::AttachWebView()
{
    assert(_window && _webviewConfiguration);
    if (!_window || !_webviewConfiguration)
        std::abort();

    NSString* initScriptSource = @"window.__receiveMessageCallbacks = [];"
			"window.__dispatchMessageCallback = function(message) {"
			"	window.__receiveMessageCallbacks.forEach(function(callback) { callback(message); });"
			"};"
			"window.external = {"
			"	sendMessage: function(message) {"
			"		window.webkit.messageHandlers.photinointerop.postMessage(message);"
			"	},"
			"	receiveMessage: function(callback) {"
			"		window.__receiveMessageCallbacks.push(callback);"
			"	}"
			"};";

    WKUserScript* initScript = [[WKUserScript alloc]
        initWithSource: initScriptSource
        injectionTime: WKUserScriptInjectionTimeAtDocumentStart
        forMainFrameOnly:YES];
    if (!initScript)
        std::abort();

    WKUserContentController* userContentController = [WKUserContentController new];
    if (!userContentController)
        std::abort();

    [userContentController addUserScript:initScript];
    [initScript release];

    _uiDelegate = [[UiDelegate alloc] init];
    if (!_uiDelegate)
        std::abort();

    _uiDelegate->photino = this;
    _uiDelegate->window = _window;
    _uiDelegate->webMessageReceivedCallback = _webMessageReceivedCallback;

    [userContentController addScriptMessageHandler:_uiDelegate name:@"photinointerop"];

    _webviewConfiguration.userContentController = userContentController;
    [userContentController release];

    _webview = [[WKWebView alloc]
        initWithFrame: _window.contentView.frame
        configuration: _webviewConfiguration];
    if (!_webview)
        std::abort();

    _navigationDelegate = [[NavigationDelegate alloc] init];
    if (!_navigationDelegate)
        std::abort();

    _navigationDelegate->photino = this;
    _navigationDelegate->window = _window;

    _webview.UIDelegate = _uiDelegate;
    _webview.navigationDelegate = _navigationDelegate;

    [_webview setAutoresizingMask: NSViewWidthSizable | NSViewHeightSizable];
    [_window.contentView addSubview: _webview];
    [_window.contentView setAutoresizesSubviews: true];

    SetUserAgent(_userAgent);

    if (!_startUrl.empty())
    {
        NavigateToUrl(_startUrl);
    }
    else if (!_startString.empty())
    {
        NavigateToString(_startString);
    }
    else
    {
        NSAlert *alert = [[[NSAlert alloc] init] autorelease];
        [alert setMessageText:@"Neither StartUrl nor StartString was specified"];
        [alert runModal];
        std::abort();
    }
}

void Photino::Show()
{
    if (_webview == nil)
        AttachWebView();

    [_window makeKeyAndOrderFront:_window];
    [_window orderFrontRegardless];
}

#endif