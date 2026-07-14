#include "Photino.h"
#include "Photino.Mac.Internal.h"
#include "Photino.Strings.h"

using namespace PhotinoX::Native;

void Photino::Close() const
{
    assert(_window);
    if (!_window) return;

    if (_chromeless)
    {
        if (!PhotinoMacIsShuttingDown() && InvokeClosing())
            return;
        // Can't use performClose because frame has no title area and close button
        [_window close];
    }
    else
    {
        // Simulates user clicking the close button
    	[_window performClose:_window];
    }
}

void Photino::SetTitle(const PlatformString& title)
{
    assert(_window);
    if (!_window) return;

    NSString* nsTitle = ToNSString(title);
    if (!nsTitle) return;

    [_window setTitle:nsTitle];
    _windowTitle = title;
}

void Photino::SetIconFile(const PlatformString& filename)
{
    assert(_window);
    if (!_window || filename.empty()) return;

    NSString* path = ToNSString(filename);
    if (!path) return;

    NSImage* icon = [[NSImage alloc] initWithContentsOfFile:path];
    if (!icon) return;

    NSButton* iconButton = [_window standardWindowButton:NSWindowDocumentIconButton];
    if (iconButton)
    {
        [iconButton setImage:icon];
        //[NSApp setApplicationIconImage:icon];
        _iconFileName = filename;
    }

    [icon release];
}