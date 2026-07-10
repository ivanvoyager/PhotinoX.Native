#ifdef __APPLE__
#import "Photino.Mac.UiDelegate.h"

#include "Photino.h"

@implementation UiDelegate

- (id)init
{
    self = [super init];
    if (self)
    {
        window = nil;
        photino = nullptr;
        webMessageReceivedCallback = nullptr;
    }

    return self;
}

- (void)userContentController:(WKUserContentController*)userContentController
        didReceiveScriptMessage:(WKScriptMessage*)message
{
    if (!webMessageReceivedCallback) return;

    if (![message.body isKindOfClass:[NSString class]]) return;

    NSString* body = (NSString*)message.body;
    const char* messageUtf8 = [body UTF8String];
    if (!messageUtf8) return;

    webMessageReceivedCallback(messageUtf8);
}

- (void)webView:(WKWebView*)webView
        runJavaScriptAlertPanelWithMessage:(NSString*)message
        initiatedByFrame:(WKFrameInfo*)frame
        completionHandler:(void (^)(void))completionHandler
{
    if (!completionHandler) return;

    NSAlert* alert = [[NSAlert alloc] init];

    [alert setMessageText:@"Alert"];
    [alert setInformativeText:message ?: @""];
    [alert addButtonWithTitle:@"OK"];

    NSWindow* targetWindow = window ?: [webView window];
    if (!targetWindow)
    {
        completionHandler();
        [alert release];
        return;
    }

    [alert beginSheetModalForWindow:targetWindow completionHandler:^(NSModalResponse response) {
        completionHandler();
        [alert release];
    }];
}

- (void)webView:(WKWebView*)webView
        runJavaScriptConfirmPanelWithMessage:(NSString*)message
        initiatedByFrame:(WKFrameInfo*)frame
        completionHandler:(void (^)(BOOL result))completionHandler
{
    if (!completionHandler) return;

    NSAlert* alert = [[NSAlert alloc] init];

    [alert setMessageText:@"Confirm"];
    [alert setInformativeText:message ?: @""];
    
    [alert addButtonWithTitle:@"OK"];
    [alert addButtonWithTitle:@"Cancel"];

    NSWindow* targetWindow = window ?: [webView window];
    if (!targetWindow)
    {
        completionHandler(NO);
        [alert release];
        return;
    }

    [alert beginSheetModalForWindow:targetWindow completionHandler:^(NSModalResponse response) {
        completionHandler(response == NSAlertFirstButtonReturn);
        [alert release];
    }];
}

- (void)webView:(WKWebView*)webView
        runJavaScriptTextInputPanelWithPrompt:(NSString*)prompt
        defaultText:(NSString*)defaultText
        initiatedByFrame:(WKFrameInfo*)frame
        completionHandler:(void (^)(NSString*result))completionHandler
{
    if (!completionHandler) return;

    NSAlert* alert = [[NSAlert alloc] init];

    [alert setMessageText:@"Prompt"];
    [alert setInformativeText:prompt ?: @""];
    
    [alert addButtonWithTitle:@"OK"];
    [alert addButtonWithTitle:@"Cancel"];
    
    NSTextField* input = [[NSTextField alloc] initWithFrame:NSMakeRect(0, 0, 200, 24)];
    [input setStringValue:defaultText ?: @""];
    [alert setAccessoryView:input];

    NSWindow* targetWindow = window ?: [webView window];
    if (!targetWindow)
    {
        completionHandler(nil);
        [input release];
        [alert release];
        return;
    }

    [alert beginSheetModalForWindow:targetWindow completionHandler:^(NSModalResponse response) {
        [input validateEditing];
        NSString* result = response == NSAlertFirstButtonReturn
            ? [[[input stringValue] retain] autorelease]
            : nil;

        completionHandler(result);

        [input release];
        [alert release];
    }];
}

- (void)webView:(WKWebView*)webView 
        runOpenPanelWithParameters:(WKOpenPanelParameters*)parameters 
        initiatedByFrame:(WKFrameInfo*)frame 
        completionHandler:(void (^)(NSArray<NSURL *> *URLs))completionHandler
{
    if (!completionHandler) return;

    if (!parameters)
    {
        completionHandler(nil);
        return;
    }

    NSOpenPanel* openDlg = [NSOpenPanel openPanel];
    [openDlg setCanChooseFiles:![parameters allowsDirectories]];
    [openDlg setCanChooseDirectories:[parameters allowsDirectories]];
    openDlg.allowsMultipleSelection = [parameters allowsMultipleSelection];
    [openDlg setPrompt:NSLocalizedString(@"OK", nil)];

    NSWindow* targetWindow = window ?: [webView window];
    if (!targetWindow)
    {
        completionHandler(nil);
        return;
    }

    [openDlg beginSheetModalForWindow:targetWindow completionHandler:^(NSModalResponse response) {
        completionHandler(response == NSModalResponseOK ? [openDlg URLs] : nil);
    }];
}

- (void)webView:(WKWebView*)webView 
        requestMediaCapturePermissionForOrigin:(WKSecurityOrigin*)origin 
        initiatedByFrame:(WKFrameInfo*)frame 
        type:(WKMediaCaptureType)type 
        decisionHandler:(void (^)(WKPermissionDecision decision))decisionHandler
{
    if (!decisionHandler) return;

    if (!photino)
    {
        decisionHandler(WKPermissionDecisionPrompt);
        return;
    }

    bool grantPermissions = false;
    photino->GetGrantBrowserPermissions(&grantPermissions);

    decisionHandler(grantPermissions ? WKPermissionDecisionGrant : WKPermissionDecisionPrompt);
}

@end

#endif