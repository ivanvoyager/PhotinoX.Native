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

#include "Dependencies/json.hpp"

using json = nlohmann::json;
using namespace PhotinoX::Native;

void Photino::GetTransparentEnabled(bool* enabled) const
{
    assert(enabled);
    if (!enabled) return;
    //! Not implemented (supported?) on macOS
    *enabled = _transparentEnabled;
}

void Photino::SetTransparentEnabled(bool enabled)
{
    _transparentEnabled = enabled;

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

    *enabled = _contextMenuEnabled;
}

void Photino::SetContextMenuEnabled(bool enabled)
{
    _contextMenuEnabled = enabled;

    //! Not supported on macOS
}

void Photino::GetZoomEnabled(bool* enabled) const
{
    assert(enabled);
    if (!enabled) return;

    *enabled = _zoomEnabled;
}

void Photino::SetZoomEnabled(bool enabled)
{
    _zoomEnabled = enabled;

    //! Not implemented (supported?) on macOS
}

void Photino::GetZoom(int* zoom) const
{
    assert(zoom);
    if (!zoom) return;

    *zoom = _zoom;

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

    _zoom = zoom;

    if (!platform_->webView) return;

    CGFloat newZoom = static_cast<CGFloat>(zoom) / 100.0;
    [platform_->webView setMagnification:newZoom];
}

void Photino::GetDevToolsEnabled(bool* enabled) const
{
    assert(enabled);
    if (!enabled) return;

    *enabled = _devToolsEnabled;
}

void Photino::SetDevToolsEnabled(bool enabled)
{
    _devToolsEnabled = enabled;

    SetPreference(@"developerExtrasEnabled", enabled ? @YES : @NO);
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

	*enabled = _ignoreCertificateErrorsEnabled;
}

void Photino::SetUserAgent(const PlatformString& userAgent)
{
    _userAgent = userAgent;

    if (!platform_->webView) return;

    NSString* nsUserAgent = ToNSString(userAgent);
    if (!nsUserAgent) return;

    [platform_->webView setCustomUserAgent:nsUserAgent];
}

// Set preferences with a string key and a value of any type
bool Photino::SetPreference(NSString* key, NSNumber* value)
{
    assert(platform_->webViewConfiguration && key && value);
    if (!platform_->webViewConfiguration || !key || !value) return false;

    @try
    {
        [platform_->webViewConfiguration.preferences setValue:value forKey:key];
        return true;
    }
    @catch (NSException* exception)
    {
        return false;
    }
}

bool Photino::SetPreference(NSString* key, NSString* value)
{
    assert(platform_->webViewConfiguration && key && value);
    if (!platform_->webViewConfiguration || !key || !value) return false;

    @try
    {
        [platform_->webViewConfiguration.preferences setValue:value forKey:key];
        return true;
    }
    @catch (NSException* exception)
    {
        return false;
    }
}

void Photino::AddCustomSchemeHandlers()
{
    assert(!platform_->webView);
    if (platform_->webView) return;

    assert(platform_->webViewConfiguration);
    if (!platform_->webViewConfiguration || !_customSchemeCallback) return;

    for (const auto& scheme : _customSchemeNames)
    {
        NSString* nsScheme = ToNSString(scheme);
        if (!nsScheme) continue;

        UrlSchemeHandler* schemeHandler = [[UrlSchemeHandler alloc] init];
        if (!schemeHandler) continue;

        schemeHandler->requestHandler = _customSchemeCallback;

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
    platform_->uiDelegate->webMessageReceivedCallback = _webMessageReceivedCallback;

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
