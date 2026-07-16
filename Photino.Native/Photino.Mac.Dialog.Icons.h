#pragma once

#ifdef __APPLE__

#import <AppKit/AppKit.h>

namespace PhotinoX::Native
{
    NSImage* CreateErrorDialogIcon();
    NSImage* CreateInfoDialogIcon();
    NSImage* CreateQuestionDialogIcon();
    NSImage* CreateWarningDialogIcon();
}

#endif