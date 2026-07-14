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

void Photino::GetPosition(int* x, int* y) const
{
    assert(x || y);
    if (!x && !y) return;

    if (x) *x = 0;
    if (y) *y = 0;

    if (!_window) return;

    NSRect frame = [_window frame];

    if (x)
        *x = static_cast<int>(roundf(frame.origin.x));

    if (y)
    {
        NSScreen* screen = [_window screen];
        if (!screen) return;

        NSRect screenFrame = [screen frame];

        int windowTop = static_cast<int>(roundf(frame.origin.y));
        int windowHeight = static_cast<int>(roundf(frame.size.height));
        int screenTop = static_cast<int>(roundf(screenFrame.origin.y));
        int screenHeight = static_cast<int>(roundf(screenFrame.size.height));

        *y = screenTop + screenHeight - (windowTop + windowHeight);
    }
}

void Photino::SetPosition(int x, int y)
{
    assert(_window);
    if (!_window) return;

    NSScreen* screen = [_window screen];
    if (!screen) return;

    NSRect screenFrame = [screen frame];
    NSRect windowFrame = [_window frame];

    CGFloat left = static_cast<CGFloat>(x);
    CGFloat top = screenFrame.origin.y
        + screenFrame.size.height
        - static_cast<CGFloat>(y)
        - windowFrame.size.height;

    [_window setFrameOrigin:NSMakePoint(left, top)];
}

void Photino::GetSize(int* width, int* height) const
{
    assert(width || height);
    if (!width && !height) return;

    if (width)  *width = 0;
    if (height) *height = 0;

    if (!_window) return;

    NSSize size = [_window frame].size;

    if (width) *width = static_cast<int>(roundf(size.width));
    if (height) *height = static_cast<int>(roundf(size.height));
}

void Photino::SetSize(int width, int height)
{
    assert(_window);
    if (!_window) return;

    if (width <= 0 || height <= 0)
        return;

    // The macOS window server has a limit of 10,000 pixels for either dimension
    // See: https://developer.apple.com/documentation/appkit/nswindow/1419595-maxsize
    width = (std::min)(width, 10000);
    height = (std::min)(height, 10000);

    NSSize minSize = [_window minSize];
    NSSize maxSize = [_window maxSize];

    CGFloat newWidth = (std::min)((std::max)(static_cast<CGFloat>(width), minSize.width), maxSize.width);
    CGFloat newHeight = (std::min)((std::max)(static_cast<CGFloat>(height), minSize.height), maxSize.height);

    NSRect frame = [_window frame];

    CGFloat oldHeight = frame.size.height;

    frame.size = NSMakeSize(newWidth, newHeight);

    // Reposition the window so that the top edge stays in the same place.
    frame.origin.y -= newHeight - oldHeight;

    [_window setFrame:frame display:YES];
}

void Photino::SetMinSize(int width, int height)
{
    assert(_window);
    if (!_window) return;

    // The macOS window server has a limit of 10,000 pixels for either dimension
    // See: https://developer.apple.com/documentation/appkit/nswindow/1419595-maxsize
    width = (std::min)((std::max)(0, width), 10000);
    height = (std::min)((std::max)(0, height), 10000);

    NSSize maxSize = [_window maxSize];

    if (maxSize.width > 0 && width > maxSize.width)
        maxSize.width = width;

    if (maxSize.height > 0 && height > maxSize.height)
        maxSize.height = height;

    [_window setMinSize:NSMakeSize(width, height)];
    [_window setMaxSize:maxSize];
}

void Photino::SetMaxSize(int width, int height)
{
    assert(_window);
    if (!_window) return;

    // The macOS window server has a limit of 10,000 pixels for either dimension
    // See: https://developer.apple.com/documentation/appkit/nswindow/1419595-maxsize
    width = width <= 0 ? 10000 : (std::min)(width, 10000);
    height = height <= 0 ? 10000 : (std::min)(height, 10000);

    NSSize minSize = [_window minSize];

    if (width < minSize.width)
        minSize.width = width;

    if (height < minSize.height)
        minSize.height = height;

    [_window setMinSize:minSize];
    [_window setMaxSize:NSMakeSize(width, height)];
}