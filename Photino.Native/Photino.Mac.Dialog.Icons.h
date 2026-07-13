#pragma once

#ifdef __APPLE__

#import <Cocoa/Cocoa.h>

namespace PhotinoX::Native
{
    NSImage* CreateErrorDialogIcon();
    NSImage* CreateInfoDialogIcon();
    NSImage* CreateQuestionDialogIcon();
    NSImage* CreateWarningDialogIcon();
} // namespace PhotinoX::Native

#endif