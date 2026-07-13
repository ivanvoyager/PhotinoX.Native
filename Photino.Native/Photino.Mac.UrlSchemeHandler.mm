#ifdef __APPLE__
#import "Photino.Mac.UrlSchemeHandler.h"

#include "Photino.Memory.h"
#include "Photino.Strings.h"

using namespace PhotinoX::Native;

@implementation UrlSchemeHandler

- (id)init
{
    self = [super init];
    if (self)
        requestHandler = nullptr;

    return self;
}

- (void)webView:(WKWebView*)webView startURLSchemeTask:(id <WKURLSchemeTask>)urlSchemeTask
{ 
    //NSAlert *alert = [[[NSAlert alloc] init] autorelease];
    //[alert setMessageText:@"startURLSchemeTask"];
    //[alert runModal];

    if (!urlSchemeTask)
        return;

    if (!requestHandler)
    {
        [urlSchemeTask didFailWithError:[NSError errorWithDomain:NSURLErrorDomain
                                                            code:NSURLErrorUnsupportedURL
                                                            userInfo:nil]];
        return;
    }

    NSURL* url = [[urlSchemeTask request] URL];
    NSString* absoluteString = [url absoluteString];

    if (!absoluteString)
    {
        [urlSchemeTask didFailWithError:[NSError errorWithDomain:NSURLErrorDomain
                                                            code:NSURLErrorBadURL
                                                            userInfo:nil]];
        return;
    }

    const char* urlUtf8 = [absoluteString UTF8String];
    if (!urlUtf8)
    {
        [urlSchemeTask didFailWithError:[NSError errorWithDomain:NSURLErrorDomain
                                                            code:NSURLErrorBadURL
                                                        userInfo:nil]];
        return;
    }

    int numBytes = 0;
    Utf8String contentType = nullptr;
    void* responseData = requestHandler(urlUtf8, &numBytes, &contentType);

    NSInteger statusCode = responseData && numBytes > 0 ? 200 : 404;

    NSString* nsContentType = contentType
        ? [NSString stringWithUTF8String:contentType]
        : nil;

    if (!nsContentType)
        nsContentType = @"application/octet-stream";

    NSDictionary* headers = @{
        @"Content-Type": nsContentType,
        @"Cache-Control": @"no-cache"
    };

    NSHTTPURLResponse* response =
        [[NSHTTPURLResponse alloc] initWithURL:url
                                   statusCode:statusCode
                                   HTTPVersion:nil
                                   headerFields:headers];

    if (!response)
    {
        FreeMemory(responseData);
        FreeString(const_cast<char*>(contentType));

        [urlSchemeTask didFailWithError:[NSError errorWithDomain:NSURLErrorDomain
                                                            code:NSURLErrorCannotParseResponse
                                                            userInfo:nil]];
        return;
    }

    [urlSchemeTask didReceiveResponse:response];
    if (responseData && numBytes > 0)
        [urlSchemeTask didReceiveData:[NSData dataWithBytes:responseData length:numBytes]];
    [urlSchemeTask didFinish];

    [response release];

    FreeMemory(responseData);
    FreeString(const_cast<char*>(contentType));
}

- (void)webView:(WKWebView*)webView stopURLSchemeTask:(id <WKURLSchemeTask>)urlSchemeTask
{

}

@end
#endif