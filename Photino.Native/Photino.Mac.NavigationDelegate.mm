#ifdef __APPLE__

#import "Photino.Mac.NavigationDelegate.h"
#import <Security/SecTrust.h>

#include "Photino.h"

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