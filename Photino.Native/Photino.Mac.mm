#ifdef __APPLE__

#import <Cocoa/Cocoa.h>
#import <WebKit/WebKit.h>

#import "Photino.Mac.NavigationDelegate.h"
#import "Photino.Mac.NSWindowBorderless.h"
#import "Photino.Mac.UiDelegate.h"
#import "Photino.Mac.WindowDelegate.h"

#include "Photino.h"
#include "Photino.Mac.State.h"
#include "Photino.Callbacks.h"
#include "Photino.Dialog.h"
#include "Photino.Strings.h"
#include "Photino.Application.h"

#include <algorithm>
#include <atomic>
#include <cassert>
#include <memory>
#include <mutex>
#include <vector>

using namespace PhotinoX::Native;

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

        if (initParams->Size != sizeof(PhotinoInitParams) ||
            initParams->AbiVersion != PhotinoInitParams::NativeAbiVersion)
        {
            NSAlert* alert = [[[NSAlert alloc] init] autorelease];

            [alert setMessageText:@"Native Initialization Failed"];
            [alert setInformativeText:[NSString stringWithFormat:
                @"Window initial parameters ABI mismatch. Passed size: %d bytes, expected size: %zu bytes. Passed ABI version: %d, expected ABI version: %d.",
                initParams->Size,
                sizeof(PhotinoInitParams),
                initParams->AbiVersion,
                PhotinoInitParams::NativeAbiVersion]];

            [alert runModal];

            std::abort();
        }

        InitializeFromInitParams(initParams);

        const auto startupWindowState = options_.windowState;
        options_.windowState = PhotinoWindowState::Normal;

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

        if (options_.chromeless)
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
        SetTitle(options_.windowTitle);
        SetIconFile(options_.iconFileName);

	    SetTopmost(initParams->Topmost);
        SetPosition(initParams->Left, initParams->Top);

        // It's important to set min/max size before setting size
        // SetSize is ensuring internally that the size is within min/max
        // but requires that min/max be set first.
        SetMinSize(initParams->MinWidth, initParams->MinHeight); // Defaults to 0,0
        SetMaxSize(initParams->MaxWidth, initParams->MaxHeight); // Defaults to 10000,10000
        SetSize(initParams->Width, initParams->Height);

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
        ConfigureWebViewPreferences();

        // Create WebView
        AttachWebView();

        dialog_ = new PhotinoDialog();

        suppressWindowStateCallbacks_ = true;

        Show();
        UpdateWindowState();

        switch (startupWindowState)
        {
        case PhotinoWindowState::Maximized:
            SetMaximized(true);
            break;
        case PhotinoWindowState::Minimized:
            SetMinimized(true);
            break;
        case PhotinoWindowState::FullScreen:
            SetFullScreen(true);
            break;
        default:
            break;
        }

        UpdateWindowState();
        suppressWindowStateCallbacks_ = false;
    }
}

Photino::~Photino()
{
    StopInteractiveWindowOperation();

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

    delete dialog_;
    dialog_ = nullptr;
    //[NSApp release];
}

#endif