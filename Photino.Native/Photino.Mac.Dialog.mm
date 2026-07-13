#ifdef __APPLE__

#import "Photino.Dialog.h"
#import "Photino.Mac.Dialog.Icons.h"

#include "Photino.Strings.h"

#include <utility>

#if defined(VSTGUI_USE_OBJC_UTTYPE)
#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>
#else
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif

using namespace PhotinoX::Native;

namespace
{
    void AddFilterPattern(NSMutableArray* fileTypes, const PlatformString& pattern)
    {
        if (pattern.empty())
            return;

        PlatformString extension = pattern;

        if (extension.starts_with("*."))
            extension.erase(0, 2);
        else if (extension.starts_with("."))
            extension.erase(0, 1);

        if (extension.empty() || extension == "*")
            return;

        NSString* nsExtension = ToNSString(extension);
        if (!nsExtension)
            return;

#ifdef VSTGUI_USE_OBJC_UTTYPE
        UTType* type = [UTType typeWithFilenameExtension:nsExtension];
        if (type)
            [fileTypes addObject:type];
#else
        [fileTypes addObject:nsExtension];
#endif
    }

    NSMutableArray* CreateAllowedFileTypes(const std::vector<PlatformString>& filters)
    {
        NSMutableArray* fileTypes = [[[NSMutableArray alloc] init] autorelease];

        for (const auto& filter : filters)
            AddFilterPattern(fileTypes, filter);

        return [fileTypes count] > 0 ? fileTypes : nil;
    }

    PlatformString PathFromUrl(NSURL* url)
    {
        if (!url)
            return {};

        NSString* path = [url path];
        if (!path)
            return {};

        const char* utf8 = [path UTF8String];
        return utf8 ? PlatformString(utf8) : PlatformString();
    }
}

PhotinoDialog::PhotinoDialog()
{
    _errorIcon = CreateErrorDialogIcon();
    _infoIcon = CreateInfoDialogIcon();
    _questionIcon = CreateQuestionDialogIcon();
    _warningIcon = CreateWarningDialogIcon();
}

PhotinoDialog::~PhotinoDialog()
{
    [_errorIcon release];
    [_infoIcon release];
    [_questionIcon release];
    [_warningIcon release];
}

std::vector<PlatformString> PhotinoDialog::ShowOpenFile(
    const PlatformString& title,
    const PlatformString& defaultPath,
    bool multiSelect,
    const std::vector<PlatformString>& filters) const
{
    NSOpenPanel* openDlg = [NSOpenPanel openPanel];

    NSString* nsTitle = ToNSString(title);
    if (nsTitle)
        [openDlg setTitle:nsTitle];

    [openDlg setCanChooseFiles:YES];
    [openDlg setCanChooseDirectories:NO];
    [openDlg setAllowsMultipleSelection:multiSelect];
    [openDlg setPrompt:@"Open"];

    if (!defaultPath.empty())
    {
        NSString* nsDefaultPath = ToNSString(defaultPath);
        if (nsDefaultPath)
            [openDlg setDirectoryURL:[NSURL fileURLWithPath:nsDefaultPath]];
    }

    NSMutableArray* fileTypes = CreateAllowedFileTypes(filters);
    if (fileTypes)
    {
#ifdef VSTGUI_USE_OBJC_UTTYPE
        [openDlg setAllowedContentTypes:fileTypes];
#else
        [openDlg setAllowedFileTypes:fileTypes];
#endif
    }

    if ([openDlg runModal] != NSModalResponseOK)
        return {};

    NSArray* files = [openDlg URLs];
    std::vector<PlatformString> result;
    result.reserve([files count]);

    for (NSURL* url in files)
    {
        PlatformString path = PathFromUrl(url);
        if (!path.empty())
            result.emplace_back(std::move(path));
    }

    return result;
}

std::vector<PlatformString> PhotinoDialog::ShowOpenFolder(
    const PlatformString& title,
    const PlatformString& defaultPath,
    bool multiSelect) const
{
    NSOpenPanel* openDlg = [NSOpenPanel openPanel];

    NSString* nsTitle = ToNSString(title);
    if (nsTitle)
        [openDlg setTitle:nsTitle];

    [openDlg setCanChooseFiles:NO];
    [openDlg setCanChooseDirectories:YES];
    [openDlg setCanCreateDirectories:YES];
    [openDlg setAllowsMultipleSelection:multiSelect];
    [openDlg setPrompt:@"Open"];

    if (!defaultPath.empty())
    {
        NSString* nsDefaultPath = ToNSString(defaultPath);
        if (nsDefaultPath)
            [openDlg setDirectoryURL:[NSURL fileURLWithPath:nsDefaultPath]];
    }

    if ([openDlg runModal] != NSModalResponseOK)
        return {};

    NSArray* files = [openDlg URLs];
    std::vector<PlatformString> result;
    result.reserve([files count]);

    for (NSURL* url in files)
    {
        PlatformString path = PathFromUrl(url);
        if (!path.empty())
            result.emplace_back(std::move(path));
    }

    return result;
}

PlatformString PhotinoDialog::ShowSaveFile(
    const PlatformString& title,
    const PlatformString& defaultPath,
    const std::vector<PlatformString>& filters,
    const PlatformString& defaultFileName) const
{
    NSSavePanel* saveDlg = [NSSavePanel savePanel];

    NSString* nsTitle = ToNSString(title);
    if (nsTitle)
        [saveDlg setTitle:nsTitle];

    [saveDlg setPrompt:@"Save"];
    [saveDlg setCanSelectHiddenExtension:YES];

    if (!defaultPath.empty())
    {
        NSString* nsDefaultPath = ToNSString(defaultPath);
        if (nsDefaultPath)
            [saveDlg setDirectoryURL:[NSURL fileURLWithPath:nsDefaultPath]];
    }

    if (!defaultFileName.empty())
    {
        NSString* nsDefaultFileName = ToNSString(defaultFileName);
        if (nsDefaultFileName)
            [saveDlg setNameFieldStringValue:nsDefaultFileName];
    }

    NSMutableArray* fileTypes = CreateAllowedFileTypes(filters);
    if (fileTypes)
    {
#ifdef VSTGUI_USE_OBJC_UTTYPE
        [saveDlg setAllowedContentTypes:fileTypes];
#else
        [saveDlg setAllowedFileTypes:fileTypes];
#endif
        [saveDlg setAllowsOtherFileTypes:NO];
    }

    if ([saveDlg runModal] != NSModalResponseOK)
        return {};

    return PathFromUrl([saveDlg URL]);
}

DialogResult PhotinoDialog::ShowMessage(
    const PlatformString& title,
    const PlatformString& text,
    DialogButtons buttons,
    DialogIcon icon) const
{
    NSAlert* alert = [[NSAlert alloc] init];
    if (!alert)
        return DialogResult::Cancel;


    NSString* nsTitle = ToNSString(title);
    NSString* nsText = ToNSString(text);

    if (!nsTitle && !nsText)
    {
        [alert release];
        return DialogResult::Cancel;
    }

    if (nsTitle && [nsTitle length] > 0)
    {
        [alert setMessageText:nsTitle];

        if (nsText)
            [alert setInformativeText:nsText];
    }
    else
    {
        [alert setMessageText:nsText ?: @""];
    }

    switch (buttons)
    {
    case DialogButtons::Ok:
        [alert addButtonWithTitle:@"OK"];
        break;
    case DialogButtons::OkCancel:
        [alert addButtonWithTitle:@"OK"];
        [alert addButtonWithTitle:@"Cancel"];
        break;
    case DialogButtons::YesNo:
        [alert addButtonWithTitle:@"Yes"];
        [alert addButtonWithTitle:@"No"];
        break;
    case DialogButtons::YesNoCancel:
        [alert addButtonWithTitle:@"Yes"];
        [alert addButtonWithTitle:@"No"];
        [alert addButtonWithTitle:@"Cancel"];
        break;
    case DialogButtons::RetryCancel:
        [alert addButtonWithTitle:@"Retry"];
        [alert addButtonWithTitle:@"Cancel"];
        break;
    case DialogButtons::AbortRetryIgnore:
        [alert addButtonWithTitle:@"Abort"];
        [alert addButtonWithTitle:@"Retry"];
        [alert addButtonWithTitle:@"Ignore"];
        break;
    default:
        [alert addButtonWithTitle:@"OK"];
        break;
    }

    switch (icon)
    {
    case DialogIcon::Error:
        [alert setIcon:_errorIcon];
        break;
    case DialogIcon::Warning:
        [alert setIcon:_warningIcon];
        break;
    case DialogIcon::Info:
        [alert setIcon:_infoIcon];
        break;
    case DialogIcon::Question:
        [alert setIcon:_questionIcon];
        break;
    default:
        break;
    }

    const auto result = [alert runModal];
    [alert release];

    switch (buttons)
    {
    case DialogButtons::Ok:
        return result == NSAlertFirstButtonReturn ? DialogResult::Ok : DialogResult::Cancel;

    case DialogButtons::OkCancel:
        return result == NSAlertFirstButtonReturn ? DialogResult::Ok : DialogResult::Cancel;

    case DialogButtons::YesNo:
        return result == NSAlertFirstButtonReturn ? DialogResult::Yes : DialogResult::No;

    case DialogButtons::YesNoCancel:
        switch (result)
        {
        case NSAlertFirstButtonReturn:
            return DialogResult::Yes;
        case NSAlertSecondButtonReturn:
            return DialogResult::No;
        default:
            return DialogResult::Cancel;
        }

    case DialogButtons::RetryCancel:
        return result == NSAlertFirstButtonReturn ? DialogResult::Retry : DialogResult::Cancel;

    case DialogButtons::AbortRetryIgnore:
        switch (result)
        {
        case NSAlertFirstButtonReturn:
            return DialogResult::Abort;
        case NSAlertSecondButtonReturn:
            return DialogResult::Retry;
        default:
            return DialogResult::Ignore;
        }

    default:
        return result == NSAlertFirstButtonReturn ? DialogResult::Ok : DialogResult::Cancel;
    }

    return DialogResult::Cancel;
}

#endif