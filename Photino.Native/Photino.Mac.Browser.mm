#import <Foundation/Foundation.h>
#import <WebKit/WebKit.h>

#import "Photino.Mac.NavigationDelegate.h"
#import "Photino.Mac.UiDelegate.h"
#import "Photino.Mac.UrlSchemeHandler.h"

#include "Photino.h"
#include "Photino.Mac.Internal.h"
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

    if (!_webview) return;

    CGFloat rawValue = [_webview magnification];
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

    if (!_webview) return;

    CGFloat newZoom = static_cast<CGFloat>(zoom) / 100.0;
    [_webview setMagnification:newZoom];
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
