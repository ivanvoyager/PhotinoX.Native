#import <AppKit/AppKit.h>

#include "Photino.Application.h"
#include "Photino.h"
#include "Photino.Mac.Internal.h"
#include "Photino.Mac.State.h"
#include "Photino.Callbacks.h"
#include "Photino.Strings.h"

using namespace PhotinoX::Native;

void* Photino::GetNSWindow() const noexcept
{
    return (__bridge void*)platform_->window;
}

void Photino::Close() const
{
    assert(platform_->window);
    if (!platform_->window) return;

    if (options_.chromeless)
    {
        if (!PhotinoApplication::Instance().IsShuttingDown() && InvokeClosing())
            return;
        // Can't use performClose because frame has no title area and close button
        [platform_->window close];
    }
    else
    {
        // Simulates user clicking the close button
    	[platform_->window performClose:platform_->window];
    }
}

void Photino::SetTitle(const PlatformString& title)
{
    assert(platform_->window);
    if (!platform_->window) return;

    NSString* nsTitle = ToNSString(title);
    if (!nsTitle) return;

    [platform_->window setTitle:nsTitle];
    options_.windowTitle = title;
}

void Photino::SetIconFile(const PlatformString& filename)
{
    assert(platform_->window);
    if (!platform_->window || filename.empty()) return;

    NSString* path = ToNSString(filename);
    if (!path) return;

    NSImage* icon = [[NSImage alloc] initWithContentsOfFile:path];
    if (!icon) return;

    NSButton* iconButton = [platform_->window standardWindowButton:NSWindowDocumentIconButton];
    if (iconButton)
    {
        [iconButton setImage:icon];
        //[NSApp setApplicationIconImage:icon];
        options_.iconFileName = filename;
    }

    [icon release];
}

void Photino::GetPosition(int* x, int* y) const
{
    assert(x || y);
    if (!x && !y) return;

    if (x) *x = 0;
    if (y) *y = 0;

    if (!platform_->window) return;

    NSRect frame = [platform_->window frame];

    if (x)
        *x = static_cast<int>(roundf(frame.origin.x));

    if (y)
    {
        NSScreen* screen = [platform_->window screen];
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
    assert(platform_->window);
    if (!platform_->window) return;

    NSScreen* screen = [platform_->window screen];
    if (!screen) return;

    NSRect screenFrame = [screen frame];
    NSRect windowFrame = [platform_->window frame];

    CGFloat left = static_cast<CGFloat>(x);
    CGFloat top = screenFrame.origin.y
        + screenFrame.size.height
        - static_cast<CGFloat>(y)
        - windowFrame.size.height;

    [platform_->window setFrameOrigin:NSMakePoint(left, top)];
}

void Photino::GetSize(int* width, int* height) const
{
    assert(width || height);
    if (!width && !height) return;

    if (width)  *width = 0;
    if (height) *height = 0;

    if (!platform_->window) return;

    NSSize size = [platform_->window frame].size;

    if (width) *width = static_cast<int>(roundf(size.width));
    if (height) *height = static_cast<int>(roundf(size.height));
}

void Photino::SetSize(int width, int height)
{
    assert(platform_->window);
    if (!platform_->window) return;

    if (width <= 0 || height <= 0)
        return;

    // The macOS window server has a limit of 10,000 pixels for either dimension
    // See: https://developer.apple.com/documentation/appkit/nswindow/1419595-maxsize
    width = (std::min)(width, 10000);
    height = (std::min)(height, 10000);

    NSSize minSize = [platform_->window minSize];
    NSSize maxSize = [platform_->window maxSize];

    CGFloat newWidth = (std::min)((std::max)(static_cast<CGFloat>(width), minSize.width), maxSize.width);
    CGFloat newHeight = (std::min)((std::max)(static_cast<CGFloat>(height), minSize.height), maxSize.height);

    NSRect frame = [platform_->window frame];

    CGFloat oldHeight = frame.size.height;

    frame.size = NSMakeSize(newWidth, newHeight);

    // Reposition the window so that the top edge stays in the same place.
    frame.origin.y -= newHeight - oldHeight;

    [platform_->window setFrame:frame display:YES];
}

void Photino::SetMinSize(int width, int height)
{
    assert(platform_->window);
    if (!platform_->window) return;

    // The macOS window server has a limit of 10,000 pixels for either dimension
    // See: https://developer.apple.com/documentation/appkit/nswindow/1419595-maxsize
    width = (std::min)((std::max)(0, width), 10000);
    height = (std::min)((std::max)(0, height), 10000);

    NSSize maxSize = [platform_->window maxSize];

    if (maxSize.width > 0 && width > maxSize.width)
        maxSize.width = width;

    if (maxSize.height > 0 && height > maxSize.height)
        maxSize.height = height;

    [platform_->window setMinSize:NSMakeSize(width, height)];
    [platform_->window setMaxSize:maxSize];
}

void Photino::SetMaxSize(int width, int height)
{
    assert(platform_->window);
    if (!platform_->window) return;

    // The macOS window server has a limit of 10,000 pixels for either dimension
    // See: https://developer.apple.com/documentation/appkit/nswindow/1419595-maxsize
    width = width <= 0 ? 10000 : (std::min)(width, 10000);
    height = height <= 0 ? 10000 : (std::min)(height, 10000);

    NSSize minSize = [platform_->window minSize];

    if (width < minSize.width)
        minSize.width = width;

    if (height < minSize.height)
        minSize.height = height;

    [platform_->window setMinSize:minSize];
    [platform_->window setMaxSize:NSMakeSize(width, height)];
}

bool Photino::Activate() const
{
    assert(platform_->window);
    if (!platform_->window) return false;

    [platform_->window makeKeyAndOrderFront:nil];
    [NSApp activateIgnoringOtherApps:YES];

    return true;
}

bool Photino::Center() const
{
    assert(platform_->window);
    if (!platform_->window) return false;

    [platform_->window center];
    return true;
}

void Photino::ApplyPendingStateAfterFullScreenExit()
{
    const auto pendingState = platform_->pendingStateAfterFullScreenExit;
    platform_->pendingStateAfterFullScreenExit = PhotinoWindowState::Normal;

    switch (pendingState)
    {
    case PhotinoWindowState::Maximized:
        Maximize();
        break;
    case PhotinoWindowState::Minimized:
        Minimize();
        break;
    default:
        break;
    }
}

void Photino::HandleFullScreenExitCompleted() noexcept
{
    if (platform_->pendingStateAfterFullScreenExit != PhotinoWindowState::Normal)
        suppressRestoredCallback_ = true;

    ApplyPendingStateAfterFullScreenExit();
    UpdateWindowState();

    suppressRestoredCallback_ = false;
}

bool Photino::Maximize()
{
    assert(platform_->window);
    if (!platform_->window) return false;

    if (IsMinimized())
        [platform_->window deminiaturize:nil];

    if (IsFullScreen())
    {
        platform_->pendingStateAfterFullScreenExit = PhotinoWindowState::Maximized;
        suppressRestoredCallback_ = true;
        [platform_->window toggleFullScreen:nil];
        return true;
    }

    if (!IsMaximized())
        [platform_->window zoom:nil];

    UpdateWindowState();

    return true;
}

bool Photino::Minimize()
{
    assert(platform_->window);
    if (!platform_->window) return false;

    if (IsFullScreen())
    {
        platform_->pendingStateAfterFullScreenExit = PhotinoWindowState::Minimized;
        suppressRestoredCallback_ = true;
        [platform_->window toggleFullScreen:nil];
        return true;
    }

    if (!IsMinimized())
        [platform_->window miniaturize:nil];

    UpdateWindowState();

    return true;
}

bool Photino::Restore()
{
    assert(platform_->window);
    if (!platform_->window) return false;

    platform_->pendingStateAfterFullScreenExit = PhotinoWindowState::Normal;
    suppressRestoredCallback_ = false;

    if ([platform_->window isMiniaturized])
        [platform_->window deminiaturize:nil];

    if ([platform_->window isZoomed])
        [platform_->window zoom:nil];

    if (IsFullScreen())
        [platform_->window toggleFullScreen:nil];

    UpdateWindowState();

    return true;
}

bool Photino::Show() const
{
    assert(platform_->window);
    if (!platform_->window) return false;

    if ([platform_->window isMiniaturized])
        [platform_->window deminiaturize:nil];

    [platform_->window makeKeyAndOrderFront:nil];
    return true;
}

void Photino::BeginWindowDrag() const
{
    // Not yet implemented on macOS. NSWindow's -performWindowDragWithEvent: needs
    // the current NSEvent (the mouse-down that began the drag), which this entry
    // point does not receive. Left as a no-op until it can be built and tested
    // against Cocoa. Windows is the currently supported platform.
}

void Photino::BeginWindowResize(PhotinoWindowEdge) const
{
    // Not yet implemented on macOS. Cocoa has no direct analogue of the Windows
    // non-client resize loop; an NSEvent-driven implementation is needed and must
    // be tested on macOS first.
}

unsigned int Photino::GetScreenDpi() const
{
    //not supported on macOS - _window's devices collection does have dpi
	//https://stackoverflow.com/questions/2621439/hot-to-get-screen-dpi-linux-mac-programaticaly
    if (!platform_->window) return 72;

    NSScreen* screen = [platform_->window screen];
    if (!screen) return 72;

    return static_cast<unsigned int>(roundf(72.0f * [screen backingScaleFactor]));
}

void Photino::GetAllMonitors(GetAllMonitorsCallback callback) const noexcept
{
    assert(callback);
    if (!callback) return;

    for (NSScreen* screen in [NSScreen screens])
    {
        Monitor props{};

        NSRect frame = [screen frame];
        props.monitor.x = static_cast<int>(roundf(frame.origin.x));
        props.monitor.y = static_cast<int>(roundf(frame.origin.y));
        props.monitor.width = static_cast<int>(roundf(frame.size.width));
        props.monitor.height = static_cast<int>(roundf(frame.size.height));

        NSRect visibleFrame = [screen visibleFrame];
        props.work.x = static_cast<int>(roundf(visibleFrame.origin.x));
        props.work.y = static_cast<int>(roundf(visibleFrame.origin.y));
        props.work.width = static_cast<int>(roundf(visibleFrame.size.width));
        props.work.height = static_cast<int>(roundf(visibleFrame.size.height));

        props.scale = [screen backingScaleFactor];

        if (!callback(&props))
            break;
    }
}

std::vector<Monitor> Photino::GetMonitors() const
{
    std::vector<Monitor> monitors;

    for (NSScreen* screen in [NSScreen screens])
    {
        NSRect monitorFrame = [screen frame];
        NSRect workFrame = [screen visibleFrame];

        Monitor monitor{};
        monitor.monitor.x = static_cast<int>(roundf(monitorFrame.origin.x));
        monitor.monitor.y = static_cast<int>(roundf(monitorFrame.origin.y));
        monitor.monitor.width = static_cast<int>(roundf(monitorFrame.size.width));
        monitor.monitor.height = static_cast<int>(roundf(monitorFrame.size.height));

        monitor.work.x = static_cast<int>(roundf(workFrame.origin.x));
        monitor.work.y = static_cast<int>(roundf(workFrame.origin.y));
        monitor.work.width = static_cast<int>(roundf(workFrame.size.width));
        monitor.work.height = static_cast<int>(roundf(workFrame.size.height));

        monitor.scale = [screen backingScaleFactor];

        monitors.push_back(monitor);
    }

    return monitors;
}

bool Photino::IsFullScreen() const noexcept
{
    if (!platform_->window)
        return false;

    return ([platform_->window styleMask] & NSWindowStyleMaskFullScreen) == NSWindowStyleMaskFullScreen;
}

bool Photino::IsMinimized() const noexcept
{
    if (!platform_->window)
        return false;

    return [platform_->window isMiniaturized];
}

bool Photino::IsMaximized() const noexcept
{
    if (!platform_->window)
        return false;

    return [platform_->window isZoomed];
}

PhotinoWindowState Photino::GetPlatformWindowState() const noexcept
{
    if (!platform_->window)
        return options_.windowState;

    if (IsMinimized())
        return PhotinoWindowState::Minimized;

    if (IsFullScreen())
        return PhotinoWindowState::FullScreen;

    if (IsMaximized())
        return PhotinoWindowState::Maximized;

    return PhotinoWindowState::Normal;
}

void Photino::SetFullScreen(bool fullScreen)
{
    assert(platform_->window);
    if (!platform_->window) return;

    const bool isFullScreen = IsFullScreen();

    if (isFullScreen == fullScreen)
    {
        UpdateWindowState();
        return;
    }

    if (fullScreen)
    {
        if (IsMinimized())
        {
            [platform_->window deminiaturize:nil];
            platform_->pendingStateAfterFullScreenExit = PhotinoWindowState::Normal;
        }
        else if (IsMaximized())
            platform_->pendingStateAfterFullScreenExit = PhotinoWindowState::Maximized;
        else
            platform_->pendingStateAfterFullScreenExit = PhotinoWindowState::Normal;
    }

    [platform_->window toggleFullScreen:nil];

    UpdateWindowState();
}

void Photino::SetMaximized(bool maximized)
{
    assert(platform_->window);
    if (!platform_->window) return;

    if (maximized)
    {
        Maximize();
        return;
    }

    if ([platform_->window isZoomed])
        [platform_->window zoom:nil];

    UpdateWindowState();
}

void Photino::SetMinimized(bool minimized)
{
    assert(platform_->window);
    if (!platform_->window) return;

    if (minimized)
    {
        Minimize();
        return;
    }

    if ([platform_->window isMiniaturized])
        [platform_->window deminiaturize:nil];

    UpdateWindowState();
}

void Photino::GetResizable(bool* resizable) const
{
    assert(resizable);
    if (!resizable) return;

    *resizable = false;

    if (!platform_->window) return;

    *resizable = ([platform_->window styleMask] & NSWindowStyleMaskResizable) == NSWindowStyleMaskResizable;
}

void Photino::SetResizable(bool resizable)
{
    assert(platform_->window);
    if (!platform_->window) return;

    NSWindowStyleMask styleMask = [platform_->window styleMask];

    if (resizable)
        styleMask |= NSWindowStyleMaskResizable;
    else
        styleMask &= ~NSWindowStyleMaskResizable;

    [platform_->window setStyleMask:styleMask];
}

void Photino::GetTopmost(bool* topmost) const
{
    assert(topmost);
    if (!topmost) return;

    *topmost = false;

    if (!platform_->window) return;

    *topmost = [platform_->window level] == NSFloatingWindowLevel;
}

void Photino::SetTopmost(bool topmost)
{
    assert(platform_->window);
    if (!platform_->window) return;

    [platform_->window setLevel:topmost ? NSFloatingWindowLevel : NSNormalWindowLevel];
}