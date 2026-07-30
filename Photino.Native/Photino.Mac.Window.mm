#import <AppKit/AppKit.h>

#include "Photino.Application.h"
#include "Photino.h"
#include "Photino.Callbacks.h"
#include "Photino.Strings.h"
#include "Photino.Mac.State.h"
#include "Photino.Mac.Debug.h"

using namespace PhotinoX::Native;

namespace
{
#ifdef PHOTINO_MAC_TRACE
    void TraceMacState(const char* source, Photino* photino)
    {
        if (!photino)
            return;

        NSWindow* window = (__bridge NSWindow*)photino->GetNSWindow();

        if (!window)
        {
            PHOTINO_MAC_LOG("[mac-state] %s: window=null\n", source);
            return;
        }

        const auto styleMask = [window styleMask];
        const bool isFullScreen = (styleMask & NSWindowStyleMaskFullScreen) == NSWindowStyleMaskFullScreen;
        const bool isZoomed = [window isZoomed];
        const bool isMiniaturized = [window isMiniaturized];

        NSRect frame = [window frame];

        PHOTINO_MAC_LOG(
            "[mac-state] %s: fullscreen=%d zoomed=%d miniaturized=%d style=0x%lx frame=(%.0f,%.0f %.0fx%.0f)\n",
            source,
            isFullScreen,
            isZoomed,
            isMiniaturized,
            static_cast<unsigned long>(styleMask),
            frame.origin.x,
            frame.origin.y,
            frame.size.width,
            frame.size.height);
    }
#else
    void TraceMacState(const char*, Photino*) {}
#endif

    bool TryGetMonitorInfo(NSScreen* screen, Monitor& monitor) noexcept
    {
        if (!screen)
            return false;

        NSRect frame = [screen frame];

        monitor.monitor.x = static_cast<int>(roundf(frame.origin.x));
        monitor.monitor.y = static_cast<int>(roundf(frame.origin.y));
        monitor.monitor.width = static_cast<int>(roundf(frame.size.width));
        monitor.monitor.height = static_cast<int>(roundf(frame.size.height));

        NSRect visibleFrame = [screen visibleFrame];

        monitor.work.x = static_cast<int>(roundf(visibleFrame.origin.x));
        monitor.work.y = static_cast<int>(roundf(visibleFrame.origin.y));
        monitor.work.width = static_cast<int>(roundf(visibleFrame.size.width));
        monitor.work.height = static_cast<int>(roundf(visibleFrame.size.height));

        monitor.scale = [screen backingScaleFactor];

        return true;
    }
}

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

    switch (pendingState)
    {
    case PhotinoWindowState::Maximized:
        if (!IsMaximized())
            [platform_->window zoom:nil];
        break;

    case PhotinoWindowState::Minimized:
        if (!IsMinimized())
            [platform_->window miniaturize:nil];
        break;

    default:
        break;
    }
}

void Photino::HandleFullScreenExitCompleted() noexcept
{
    TraceMacState("HandleFullScreenExitCompleted:before", this);

    const auto pendingState = platform_->pendingStateAfterFullScreenExit;

    PHOTINO_MAC_LOG("[mac-state] pendingAfterExit=%d\n", static_cast<int>(pendingState));

    if (pendingState == PhotinoWindowState::Maximized)
    {
        suppressRestoredCallback_ = true;
        platform_->isFullScreenTransitioning = true;

        ApplyPendingStateAfterFullScreenExit();

        TraceMacState("HandleFullScreenExitCompleted:after-apply-pending", this);

        platform_->pendingStateAfterFullScreenExit = PhotinoWindowState::Normal;
        platform_->isFullScreenTransitioning = false;

        UpdateWindowState();

        TraceMacState("HandleFullScreenExitCompleted:after-update", this);

        suppressRestoredCallback_ = false;
        return;
    }

    if (pendingState == PhotinoWindowState::Minimized)
    {
        suppressRestoredCallback_ = true;
        platform_->isFullScreenTransitioning = true;

        ApplyPendingStateAfterFullScreenExit();

        TraceMacState("HandleFullScreenExitCompleted:after-apply-pending", this);

        return;
    }

    platform_->isFullScreenTransitioning = false;

    UpdateWindowState();

    TraceMacState("HandleFullScreenExitCompleted:after-normal-update", this);
}

void Photino::HandleMiniaturizeStarted() noexcept
{
    if (!platform_->window) return;

    if (IsFullScreen())
        return;

    platform_->stateBeforeMinimize =
        IsMaximized()
            ? PhotinoWindowState::Maximized
            : PhotinoWindowState::Normal;
}

void Photino::HandleMiniaturizeCompleted() noexcept
{
    if (platform_->isFullScreenTransitioning &&
        platform_->pendingStateAfterFullScreenExit == PhotinoWindowState::Minimized)
    {
        platform_->pendingStateAfterFullScreenExit = PhotinoWindowState::Normal;
        platform_->isFullScreenTransitioning = false;

        UpdateWindowState();

        suppressRestoredCallback_ = false;
        return;
    }

    UpdateWindowState();
}

bool Photino::HasPendingStateAfterFullScreenExit() const noexcept
{
    return platform_->pendingStateAfterFullScreenExit != PhotinoWindowState::Normal;
}

bool Photino::IsFullScreenTransitioning() const noexcept
{
    return platform_->isFullScreenTransitioning;
}

void Photino::SetFullScreenTransitioning(bool value) noexcept
{
    platform_->isFullScreenTransitioning = value;
}

bool Photino::Maximize()
{
    TraceMacState("Maximize:entry", this);

    assert(platform_->window);
    if (!platform_->window) return false;

    if (IsMinimized())
        [platform_->window deminiaturize:nil];

    if (IsFullScreen())
    {
        platform_->pendingStateAfterFullScreenExit = PhotinoWindowState::Maximized;
        suppressRestoredCallback_ = true;
        platform_->isFullScreenTransitioning = true;
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
    TraceMacState("Minimize:entry", this);

    assert(platform_->window);
    if (!platform_->window) return false;

    if (IsFullScreen())
    {
        platform_->pendingStateAfterFullScreenExit = PhotinoWindowState::Minimized;
        suppressRestoredCallback_ = true;
        platform_->isFullScreenTransitioning = true;
        [platform_->window toggleFullScreen:nil];
        return true;
    }

    if (!IsMinimized())
    {
        platform_->stateBeforeMinimize =
            IsMaximized()
                ? PhotinoWindowState::Maximized
                : PhotinoWindowState::Normal;

        [platform_->window miniaturize:nil];
    }

    UpdateWindowState();

    return true;
}

bool Photino::Restore()
{
    TraceMacState("Restore:entry", this);

    assert(platform_->window);
    if (!platform_->window) return false;

    suppressRestoredCallback_ = false;

    if (IsFullScreen())
    {
        platform_->isFullScreenTransitioning = true;
        [platform_->window toggleFullScreen:nil];
        return true;
    }

    platform_->pendingStateAfterFullScreenExit = PhotinoWindowState::Normal;

    const bool wasMinimized = IsMinimized();

    if (wasMinimized)
        [platform_->window deminiaturize:nil];

    if (!wasMinimized && IsMaximized())
        [platform_->window zoom:nil];

    UpdateWindowState();

    return true;
}

bool Photino::Show() const
{
    assert(platform_->window);
    if (!platform_->window) return false;

    if (IsMinimized())
        [platform_->window deminiaturize:nil];

    [platform_->window makeKeyAndOrderFront:nil];
    return true;
}

bool Photino::CanBeginResize() const noexcept
{
    return platform_->window &&
           options_.resizable &&
           !IsFullScreen() &&
           !IsMaximized();
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
    // DPI is not directly supported on macOS; use backing scale factor.
	//https://stackoverflow.com/questions/2621439/hot-to-get-screen-dpi-linux-mac-programaticaly
    if (!platform_->window) return 72;

    NSScreen* screen = [platform_->window screen];
    if (!screen) return 72;

    return static_cast<unsigned int>(roundf(72.0f * [screen backingScaleFactor]));
}

bool Photino::GetAllMonitors(GetAllMonitorsCallback callback, void* state) const noexcept
{
    assert(callback);
    if (!callback) return false;

    for (NSScreen* screen in [NSScreen screens])
    {
        Monitor props{};

        if (!TryGetMonitorInfo(screen, props))
            continue;

        if (!callback(&props, state))
            break;
    }

    return true;
}

bool Photino::GetWindowMonitor(Monitor& monitor) const noexcept
{
    assert(platform_->window);
    if (!platform_->window)
        return false;

    NSScreen* screen = [platform_->window screen];

    if (!screen)
        screen = [NSScreen mainScreen];

    if (!screen)
        screen = [[NSScreen screens] firstObject];

    return TryGetMonitorInfo(screen, monitor);
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

    if (platform_->isFullScreenTransitioning &&
        platform_->pendingStateAfterFullScreenExit != PhotinoWindowState::Normal)
    {
        return platform_->pendingStateAfterFullScreenExit;
    }

    if (IsMaximized())
        return PhotinoWindowState::Maximized;

    return PhotinoWindowState::Normal;
}

void Photino::SetFullScreen(bool fullScreen)
{
    TraceMacState("SetFullScreen:before", this);

    assert(platform_->window);
    if (!platform_->window) return;

    const bool isFullScreen = IsFullScreen();

    if (isFullScreen == fullScreen)
    {
        UpdateWindowState();
        TraceMacState("SetFullScreen:skip-after-update", this);
        return;
    }

    if (fullScreen)
    {
        if (IsMinimized())
        {
            platform_->pendingStateAfterFullScreenExit =
                platform_->stateBeforeMinimize == PhotinoWindowState::Maximized
                    ? PhotinoWindowState::Maximized
                    : PhotinoWindowState::Normal;
        }
        else if (IsMaximized())
        {
            platform_->pendingStateAfterFullScreenExit = PhotinoWindowState::Maximized;
        }
        else
        {
            platform_->pendingStateAfterFullScreenExit = PhotinoWindowState::Normal;
        }
    }

    TraceMacState("SetFullScreen:before-toggle", this);

    platform_->isFullScreenTransitioning = true;
    
    if (fullScreen && IsMinimized())
        [platform_->window deminiaturize:nil];

    [platform_->window toggleFullScreen:nil];

    TraceMacState("SetFullScreen:after-toggle", this);

    // Do not call UpdateWindowState() here.
    // AppKit reports transient intermediate window states while toggling fullscreen.
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

    if (IsFullScreen() || IsFullScreenTransitioning())
        return;

    if (IsMaximized())
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

    if (IsFullScreen() || IsFullScreenTransitioning())
        return;

    if (IsMinimized())
        [platform_->window deminiaturize:nil];

    UpdateWindowState();
}

void Photino::GetResizable(bool* resizable) const
{
    assert(resizable);
    if (!resizable) return;

    *resizable = options_.resizable;
}

void Photino::SetResizable(bool resizable)
{
    assert(platform_->window);
    if (!platform_->window) return;

    options_.resizable = resizable;

    if (options_.chromeless)
        return;

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