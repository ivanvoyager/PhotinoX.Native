#import <Foundation/Foundation.h>
#import <WebKit/WebKit.h>

#import "Photino.Mac.NavigationDelegate.h"
#import "Photino.Mac.UiDelegate.h"
#import "Photino.Mac.UrlSchemeHandler.h"

#include "Photino.h"
#include "Photino.Mac.Internal.h"
#include "Photino.Mac.State.h"
#include "Photino.Callbacks.h"
#include "Photino.Strings.h"

#include <cassert>

#include "Dependencies/json.hpp"

using json = nlohmann::json;
using namespace PhotinoX::Native;

namespace
{
    bool SetPreference(WKWebViewConfiguration* configuration, NSString* key, NSNumber* value)
    {
        assert(configuration && key && value);
        if (!configuration || !key || !value) return false;

        @try
        {
            [configuration.preferences setValue:value forKey:key];
            return true;
        }
        @catch (NSException* exception)
        {
            return false;
        }
    }

    bool SetPreference(WKWebViewConfiguration* configuration, NSString* key, NSString* value)
    {
        assert(configuration && key && value);
        if (!configuration || !key || !value) return false;

        @try
        {
            [configuration.preferences setValue:value forKey:key];
            return true;
        }
        @catch (NSException* exception)
        {
            return false;
        }
    }
}

void Photino::ConfigureWebViewPreferences(const PhotinoInitParams* initParams)
{
    assert(initParams);
    if (!initParams) return;

    SetPreference(platform_->webViewConfiguration, @"developerExtrasEnabled", initParams->DevToolsEnabled ? @YES : @NO);
    SetPreference(platform_->webViewConfiguration, @"allowFileAccessFromFileURLs", initParams->FileSystemAccessEnabled ? @YES : @NO);
    SetPreference(platform_->webViewConfiguration, @"webSecurityEnabled", initParams->WebSecurityEnabled ? @YES : @NO);
    SetPreference(platform_->webViewConfiguration, @"javaScriptCanAccessClipboard", initParams->JavascriptClipboardAccessEnabled ? @YES : @NO);
    SetPreference(platform_->webViewConfiguration, @"mediaStreamEnabled", initParams->MediaStreamEnabled ? @YES : @NO);

    SetPreference(platform_->webViewConfiguration, @"mediaDevicesEnabled", @YES);
    SetPreference(platform_->webViewConfiguration, @"mediaCaptureRequiresSecureConnection", @NO);

    if ([NSProcessInfo.processInfo isOperatingSystemAtLeastVersion:NSOperatingSystemVersion({13, 3, 0})])
    {
        SetPreference(platform_->webViewConfiguration, @"notificationEventEnabled", @YES);
    }

    SetPreference(platform_->webViewConfiguration, @"notificationsEnabled", @YES);
    SetPreference(platform_->webViewConfiguration, @"screenCaptureEnabled", @YES);

    if (browserControlInitParameters_.empty())
        return;

    json wkPreferences = json::parse(browserControlInitParameters_, nullptr, false);
    if (wkPreferences.is_discarded() || !wkPreferences.is_object())
        std::abort();

    for (json::iterator it = wkPreferences.begin(); it != wkPreferences.end(); ++it)
    {
        std::string key = it.key();
        json value = it.value();

        NSString* preferenceKey = [NSString stringWithUTF8String:key.c_str()];
        if (!preferenceKey) continue;

        if (value.is_number_integer())
        {
            SetPreference(platform_->webViewConfiguration, preferenceKey, [NSNumber numberWithInt:value.get<int>()]);
        }
        else if (value.is_number_float())
        {
            SetPreference(platform_->webViewConfiguration, preferenceKey, [NSNumber numberWithDouble:value.get<double>()]);
        }
        else if (value.is_boolean())
        {
            SetPreference(platform_->webViewConfiguration, preferenceKey, [NSNumber numberWithBool:value.get<bool>()]);
        }
        else if (value.is_string())
        {
            std::string stringValue = value.get<std::string>();
            NSString* preferenceValue = [[NSString alloc] initWithUTF8String:stringValue.c_str()];
            if (preferenceValue)
            {
                SetPreference(platform_->webViewConfiguration, preferenceKey, preferenceValue);
                [preferenceValue release];
            }
        }
    }
}

void Photino::GetTransparentEnabled(bool* enabled) const
{
    assert(enabled);
    if (!enabled) return;
    //! Not implemented (supported?) on macOS
    *enabled = transparentEnabled_;
}

void Photino::SetTransparentEnabled(bool enabled)
{
    transparentEnabled_ = enabled;

    //! Not implemented (supported?) on macOS
}

void Photino::ClearBrowserAutoFill() const
{
    //TODO
}

void Photino::NavigateToString(const PlatformString& content) const
{
    assert(platform_->webView);
    if (!platform_->webView) return;

    NSString* nsContent = ToNSString(content);
    if (!nsContent) return;

    [platform_->webView loadHTMLString:nsContent baseURL:nil];
}

void Photino::NavigateToUrl(const PlatformString& url) const
{
    assert(platform_->webView);
    if (!platform_->webView || url.empty()) return;

    NSString* nsUrlString = ToNSString(url);
    if (!nsUrlString) return;

    NSURL* nsUrl = [NSURL URLWithString:nsUrlString];
    if (!nsUrl) return;

    NSURLRequest* nsRequest = [NSURLRequest requestWithURL:nsUrl];
    if (!nsRequest) return;

    [platform_->webView loadRequest:nsRequest];
}

void Photino::SendWebMessage(const PlatformString& message) const
{
    assert(platform_->webView);
    if (!platform_->webView) return;

    PlatformString script;
    script.append("__dispatchMessageCallback(");
    script.append(json(message).dump(-1, ' ', false, json::error_handler_t::replace));
    script.append(")");

    NSString* javaScriptToEval = ToNSString(script);
    if (!javaScriptToEval) return;

    [platform_->webView evaluateJavaScript:javaScriptToEval completionHandler:nil];
}

void Photino::GetContextMenuEnabled(bool* enabled) const
{
    assert(enabled);
    if (!enabled) return;

    *enabled = contextMenuEnabled_;
}

void Photino::SetContextMenuEnabled(bool enabled)
{
    contextMenuEnabled_ = enabled;

    //! Not supported on macOS
}

void Photino::GetZoomEnabled(bool* enabled) const
{
    assert(enabled);
    if (!enabled) return;

    *enabled = zoomEnabled_;
}

void Photino::SetZoomEnabled(bool enabled)
{
    zoomEnabled_ = enabled;

    //! Not implemented (supported?) on macOS
}

void Photino::GetZoom(int* zoom) const
{
    assert(zoom);
    if (!zoom) return;

    *zoom = zoom_;

    if (!platform_->webView) return;

    CGFloat rawValue = [platform_->webView magnification];
    rawValue = (rawValue * 100.0) + 0.5;
    *zoom = static_cast<int>(rawValue);
}

void Photino::SetZoom(int zoom)
{
    if (zoom < 25)
        zoom = 25;
    else if (zoom > 500)
        zoom = 500;

    zoom_ = zoom;

    if (!platform_->webView) return;

    CGFloat newZoom = static_cast<CGFloat>(zoom) / 100.0;
    [platform_->webView setMagnification:newZoom];
}

void Photino::GetDevToolsEnabled(bool* enabled) const
{
    assert(enabled);
    if (!enabled) return;

    *enabled = devToolsEnabled_;
}

void Photino::SetDevToolsEnabled(bool enabled)
{
    devToolsEnabled_ = enabled;

    SetPreference(platform_->webViewConfiguration, @"developerExtrasEnabled", enabled ? @YES : @NO);
}

void Photino::GetGrantBrowserPermissions(bool* enabled) const
{
    assert(enabled);
    if (!enabled) return;

    *enabled = grantBrowserPermissions_;
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

    *enabled = fileSystemAccessEnabled_;
}

void Photino::GetWebSecurityEnabled(bool* enabled) const
{
    assert(enabled);
    if (!enabled) return;

    *enabled = webSecurityEnabled_;
}

void Photino::GetJavascriptClipboardAccessEnabled(bool* enabled) const
{
    assert(enabled);
    if (!enabled) return;

    *enabled = javascriptClipboardAccessEnabled_;
}

void Photino::GetMediaStreamEnabled(bool* enabled) const
{
    assert(enabled);
    if (!enabled) return;

    *enabled = mediaStreamEnabled_;
}

//! Not supported on macOS. This is always false.
void Photino::GetSmoothScrollingEnabled(bool* enabled) const
{
    assert(enabled);
    if (!enabled) return;

    *enabled = false;
}

void Photino::GetIgnoreCertificateErrorsEnabled(bool* enabled) const
{
    assert(enabled);
    if (!enabled) return;

	*enabled = ignoreCertificateErrorsEnabled_;
}

void Photino::SetUserAgent(const PlatformString& userAgent)
{
    userAgent_ = userAgent;

    if (!platform_->webView) return;

    NSString* nsUserAgent = ToNSString(userAgent);
    if (!nsUserAgent) return;

    [platform_->webView setCustomUserAgent:nsUserAgent];
}

void Photino::AddCustomSchemeHandlers()
{
    assert(!platform_->webView);
    if (platform_->webView) return;

    assert(platform_->webViewConfiguration);
    if (!platform_->webViewConfiguration || !customSchemeCallback_) return;

    for (const auto& scheme : customSchemeNames_)
    {
        NSString* nsScheme = ToNSString(scheme);
        if (!nsScheme) continue;

        UrlSchemeHandler* schemeHandler = [[UrlSchemeHandler alloc] init];
        if (!schemeHandler) continue;

        schemeHandler->requestHandler = customSchemeCallback_;

        @try
        {
            [platform_->webViewConfiguration setURLSchemeHandler:schemeHandler forURLScheme:nsScheme];
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
    if (!platform_->webViewConfiguration) return true;

    if (platform_->webView) return false;

    return true;
}

void Photino::AttachWebView()
{
    assert(platform_->window && platform_->webViewConfiguration);
    if (!platform_->window || !platform_->webViewConfiguration)
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

    platform_->uiDelegate = [[UiDelegate alloc] init];
    if (!platform_->uiDelegate)
        std::abort();

    platform_->uiDelegate->photino = this;
    platform_->uiDelegate->window = platform_->window;
    platform_->uiDelegate->webMessageReceivedCallback = webMessageReceivedCallback_;

    [userContentController addScriptMessageHandler:platform_->uiDelegate name:@"photinointerop"];

    platform_->webViewConfiguration.userContentController = userContentController;
    [userContentController release];

    platform_->webView = [[WKWebView alloc]
        initWithFrame: platform_->window.contentView.frame
        configuration: platform_->webViewConfiguration];
    if (!platform_->webView)
        std::abort();

    platform_->navigationDelegate = [[NavigationDelegate alloc] init];
    if (!platform_->navigationDelegate)
        std::abort();

    platform_->navigationDelegate->photino = this;
    platform_->navigationDelegate->window = platform_->window;

    platform_->webView.UIDelegate = platform_->uiDelegate;
    platform_->webView.navigationDelegate = platform_->navigationDelegate;

    [platform_->webView setAutoresizingMask: NSViewWidthSizable | NSViewHeightSizable];
    [platform_->window.contentView addSubview: platform_->webView];
    [platform_->window.contentView setAutoresizesSubviews: true];

    SetUserAgent(userAgent_);

    if (!startUrl_.empty())
    {
        NavigateToUrl(startUrl_);
    }
    else if (!startString_.empty())
    {
        NavigateToString(startString_);
    }
    else
    {
        NSAlert *alert = [[[NSAlert alloc] init] autorelease];
        [alert setMessageText:@"Neither StartUrl nor StartString was specified"];
        [alert runModal];
        std::abort();
    }
}
