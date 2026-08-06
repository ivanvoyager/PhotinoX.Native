#ifdef __APPLE__

#import "Photino.Mac.NavigationDelegate.h"
#import <Security/SecTrust.h>

#include "Photino.h"

using namespace PhotinoX::Native;

@implementation NavigationDelegate : NSObject

- (id)init
{
    self = [super init];
    if (self)
    {
        window = nil;
        photino = nullptr;
    }

    return self;
}

- (void)webView:(WKWebView*)webView
    decidePolicyForNavigationAction:(WKNavigationAction*)navigationAction
    decisionHandler:(void (^)(WKNavigationActionPolicy))decisionHandler
{
    if (!decisionHandler)
        return;

    if (!photino || !navigationAction)
    {
        decisionHandler(WKNavigationActionPolicyAllow);
        return;
    }

    WKFrameInfo* targetFrame = navigationAction.targetFrame;

    // New-window navigations are handled by WKUIDelegate.
    if (!targetFrame)
    {
        decisionHandler(WKNavigationActionPolicyAllow);
        return;
    }

    // This event is for top-level navigation only.
    if (![targetFrame isMainFrame])
    {
        decisionHandler(WKNavigationActionPolicyAllow);
        return;
    }

    NSURL* url = navigationAction.request.URL;
    NSString* uri = url.absoluteString;
    const char* uriUtf8 = uri ? [uri UTF8String] : nullptr;

    bool cancel = photino->InvokeNavigationStarting(uriUtf8 && *uriUtf8 ? PlatformString(uriUtf8) : PlatformString("about:blank"));

    decisionHandler(cancel ? WKNavigationActionPolicyCancel : WKNavigationActionPolicyAllow);
}

- (void)webView:(WKWebView*)webView
    didCommitNavigation:(WKNavigation*)navigation
{
    if (!photino) return;

    NSURL* url = webView.URL;
    NSString* uri = url.absoluteString;
    const char* uriUtf8 = uri ? [uri UTF8String] : nullptr;

    photino->InvokeContentLoading(uriUtf8 && *uriUtf8 ? PlatformString(uriUtf8) : PlatformString("about:blank"));
}

- (void)webView:(WKWebView*)webView
    didFinishNavigation:(WKNavigation*)navigation
{
    if (!photino) return;

    NSURL* url = webView.URL;
    NSString* uri = url.absoluteString;
    const char* uriUtf8 = uri ? [uri UTF8String] : nullptr;

    photino->InvokeContentLoaded(uriUtf8 && *uriUtf8 ? PlatformString(uriUtf8) : PlatformString("about:blank"));
}

- (void)webView:(WKWebView*)webView
    didReceiveAuthenticationChallenge:(NSURLAuthenticationChallenge*)challenge
    completionHandler:(void (^)(NSURLSessionAuthChallengeDisposition disposition, NSURLCredential* credential))completionHandler
{
    if (!challenge || !completionHandler)
        return;

    if (!photino)
    {
        completionHandler(NSURLSessionAuthChallengePerformDefaultHandling, nil);
        return;
    }

    NSString* authenticationMethod = challenge.protectionSpace.authenticationMethod;
    if (![authenticationMethod isEqualToString:NSURLAuthenticationMethodServerTrust])
    {
        completionHandler(NSURLSessionAuthChallengePerformDefaultHandling, nil);
        return;
    }

    bool ignoreCertificateErrorsEnabled = false;
    photino->GetIgnoreCertificateErrorsEnabled(&ignoreCertificateErrorsEnabled);

    if (!ignoreCertificateErrorsEnabled)
    {
        completionHandler(NSURLSessionAuthChallengePerformDefaultHandling, nil);
        return;
    }

    SecTrustRef serverTrust = challenge.protectionSpace.serverTrust;
    if (!serverTrust)
    {
        completionHandler(NSURLSessionAuthChallengePerformDefaultHandling, nil);
        return;
    }

    completionHandler(
        NSURLSessionAuthChallengeUseCredential,
        [NSURLCredential credentialForTrust:serverTrust]);
}

@end
#endif