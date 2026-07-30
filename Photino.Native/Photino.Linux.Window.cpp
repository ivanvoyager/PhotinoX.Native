#include "Photino.h"
#include "Photino.Enums.h"
#include "Photino.Callbacks.h"
#include "Photino.Strings.h"
#include "Photino.Linux.State.h"
#include "Photino.Linux.Debug.h"

#include <gtk/gtk.h>

#include <algorithm>
#include <cassert>

using namespace PhotinoX::Native;

namespace
{
#ifdef PHOTINO_LINUX_TRACE
    const char* ToString(PhotinoWindowState state)
    {
        switch (state)
        {
        case PhotinoWindowState::Normal:
            return "Normal";
        case PhotinoWindowState::Minimized:
            return "Minimized";
        case PhotinoWindowState::Maximized:
            return "Maximized";
        case PhotinoWindowState::FullScreen:
            return "FullScreen";
        default:
            return "Unknown";
        }
    }

    void TraceLinuxState(const char* source, Photino* photino)
    {
        if (!photino)
            return;

        auto widget = static_cast<GtkWidget*>(photino->GetGtkWidget());

        if (!widget)
        {
            PHOTINO_LINUX_LOG("[linux-state] %s: widget=null\n", source);
            return;
        }

        GdkWindow* gdkWindow = gtk_widget_get_window(widget);

        if (!gdkWindow)
        {
            PHOTINO_LINUX_LOG("[linux-state] %s: gdkWindow=null\n", source);
            return;
        }

        const GdkWindowState rawState = gdk_window_get_state(gdkWindow);

        gint x = 0;
        gint y = 0;
        gint width = 0;
        gint height = 0;

        gtk_window_get_position(GTK_WINDOW(widget), &x, &y);
        gtk_window_get_size(GTK_WINDOW(widget), &width, &height);

        gint originX = 0;
        gint originY = 0;
        gdk_window_get_origin(gdkWindow, &originX, &originY);

        GdkRectangle frameExtents{};
        gdk_window_get_frame_extents(gdkWindow, &frameExtents);

        PhotinoWindowState logicalState{};
        photino->GetWindowState(&logicalState);

        PHOTINO_LINUX_LOG(
            "[linux-state] %s: logical=%s fullscreen=%d maximized=%d minimized=%d focused=%d tiled=%d logicalMinimized=%d raw=0x%x pos=(%d,%d) origin=(%d,%d) frameExtents=(%d,%d %dx%d) size=%dx%d\n",
            source,
            ToString(logicalState),
            (rawState & GDK_WINDOW_STATE_FULLSCREEN) != 0,
            (rawState & GDK_WINDOW_STATE_MAXIMIZED) != 0,
            (rawState & GDK_WINDOW_STATE_ICONIFIED) != 0,
            (rawState & GDK_WINDOW_STATE_FOCUSED) != 0,
            (rawState & GDK_WINDOW_STATE_TILED) != 0,
            photino->IsMinimized(),
            static_cast<unsigned int>(rawState),
            x,
            y,
            originX,
            originY,
            frameExtents.x,
            frameExtents.y,
            frameExtents.width,
            frameExtents.height,
            width,
            height);
    }
#else
    void TraceLinuxState(const char*, Photino*) {}
#endif

    gboolean restore_normal_geometry_idle(gpointer data)
    {
        auto instance = static_cast<Photino*>(data);

        if (!instance)
            return G_SOURCE_REMOVE;

        instance->CompleteScheduledRestoreNormalGeometry();
        return G_SOURCE_REMOVE;
    }

    bool TryGetMonitorInfo(GdkMonitor* gdkMonitor, Monitor& monitor) noexcept
    {
        if (!gdkMonitor)
            return false;

        GdkRectangle monitorArea{};
        GdkRectangle workArea{};

        gdk_monitor_get_geometry(gdkMonitor, &monitorArea);
        gdk_monitor_get_workarea(gdkMonitor, &workArea);

        monitor.monitor.x = monitorArea.x;
        monitor.monitor.y = monitorArea.y;
        monitor.monitor.width = monitorArea.width;
        monitor.monitor.height = monitorArea.height;

        monitor.work.x = workArea.x;
        monitor.work.y = workArea.y;
        monitor.work.width = workArea.width;
        monitor.work.height = workArea.height;

        monitor.scale = gdk_monitor_get_scale_factor(gdkMonitor);

        return true;
    }

} //namespace

void* Photino::GetGtkWidget() const noexcept { return platform_->window; }

void Photino::Close() const
{
    assert(platform_->window);
    if (!platform_->window) return;

    gtk_window_close(GTK_WINDOW(platform_->window));
}

void Photino::SetTitle(const PlatformString& title)
{
    assert(platform_->window);
    if (!platform_->window) return;

    gtk_window_set_title(GTK_WINDOW(platform_->window), title.c_str());
    options_.windowTitle = title;
}

void Photino::SetIconFile(const PlatformString& filename)
{
    assert(platform_->window);
    if (!platform_->window || filename.empty()) return;

    GError* error = nullptr;
    if (gtk_window_set_icon_from_file(GTK_WINDOW(platform_->window), filename.c_str(), &error))
    {
        options_.iconFileName = filename;
        return;
    }

    if (error)
    {
        g_warning("Failed to set window icon: %s", error->message);
        g_error_free(error);
    }
}

void Photino::GetPosition(int* x, int* y) const
{
    assert(x || y);
    if (!x && !y) return;

    if (x) *x = 0;
    if (y) *y = 0;

    if (!platform_->window) return;

    gint windowX = 0;
    gint windowY = 0;
    gtk_window_get_position(GTK_WINDOW(platform_->window), &windowX, &windowY);

    if (x) *x = windowX;
    if (y) *y = windowY;
}

void Photino::SetPosition(int x, int y)
{
    assert(platform_->window);
    if (!platform_->window) return;

    gtk_window_move(GTK_WINDOW(platform_->window), x, y);
}

void Photino::GetSize(int* width, int* height) const
{
    assert(width || height);
    if (!width && !height) return;

    if (width) *width = 0;
    if (height) *height = 0;

    if (!platform_->window) return;

    gint windowWidth = 0;
    gint windowHeight = 0;
    gtk_window_get_size(GTK_WINDOW(platform_->window), &windowWidth, &windowHeight);

    if (width) *width = windowWidth;
    if (height) *height = windowHeight;
}

void Photino::SetSize(int width, int height)
{
    assert(platform_->window);
    if (!platform_->window) return;

    if (width <= 0 || height <= 0) return;

    int newWidth = width;
    int newHeight = height;

    if (platform_->sizeLimits.minWidth > 0 && newWidth < platform_->sizeLimits.minWidth)    newWidth = platform_->sizeLimits.minWidth;
    if (platform_->sizeLimits.minHeight > 0 && newHeight < platform_->sizeLimits.minHeight) newHeight = platform_->sizeLimits.minHeight;
    if (platform_->sizeLimits.maxWidth > 0 && newWidth > platform_->sizeLimits.maxWidth)    newWidth = platform_->sizeLimits.maxWidth;
    if (platform_->sizeLimits.maxHeight > 0 && newHeight > platform_->sizeLimits.maxHeight) newHeight = platform_->sizeLimits.maxHeight;

    gtk_window_resize(GTK_WINDOW(platform_->window), newWidth, newHeight);
}

void Photino::SetMinSize(int width, int height)
{
    platform_->sizeLimits.minWidth = (std::max)(0, width);
    platform_->sizeLimits.minHeight = (std::max)(0, height);

    if (platform_->sizeLimits.maxWidth > 0 && platform_->sizeLimits.minWidth > platform_->sizeLimits.maxWidth)
        platform_->sizeLimits.maxWidth = platform_->sizeLimits.minWidth;

    if (platform_->sizeLimits.maxHeight > 0 && platform_->sizeLimits.minHeight > platform_->sizeLimits.maxHeight)
        platform_->sizeLimits.maxHeight = platform_->sizeLimits.minHeight;

    ApplyGeometryHints();
}

void Photino::SetMaxSize(int width, int height)
{

    platform_->sizeLimits.maxWidth = (std::max)(0, width);
    platform_->sizeLimits.maxHeight = (std::max)(0, height);

    if (platform_->sizeLimits.maxWidth > 0 && platform_->sizeLimits.maxWidth < platform_->sizeLimits.minWidth)
        platform_->sizeLimits.minWidth = platform_->sizeLimits.maxWidth;

    if (platform_->sizeLimits.maxHeight > 0 && platform_->sizeLimits.maxHeight < platform_->sizeLimits.minHeight)
        platform_->sizeLimits.minHeight = platform_->sizeLimits.maxHeight;

    ApplyGeometryHints();
}

void Photino::ApplyGeometryHints()
{
    if (!platform_->window) return;

    platform_->hints.min_width = platform_->sizeLimits.minWidth;
    platform_->hints.min_height = platform_->sizeLimits.minHeight;
    platform_->hints.max_width = platform_->sizeLimits.maxWidth > 0 ? platform_->sizeLimits.maxWidth : G_MAXINT;
    platform_->hints.max_height = platform_->sizeLimits.maxHeight > 0 ? platform_->sizeLimits.maxHeight : G_MAXINT;

    gtk_window_set_geometry_hints(
        GTK_WINDOW(platform_->window),
        nullptr,
        &platform_->hints,
        static_cast<GdkWindowHints>(GDK_HINT_MIN_SIZE | GDK_HINT_MAX_SIZE));
}

void Photino::SaveNormalGeometry()
{
    if (!platform_->window)
        return;

    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;

    GetPosition(&x, &y);
    GetSize(&width, &height);

    if (width <= 0 || height <= 0)
        return;

    platform_->normalGeometry.left = x;
    platform_->normalGeometry.top = y;
    platform_->normalGeometry.width = width;
    platform_->normalGeometry.height = height;
    platform_->hasNormalGeometry = true;

    PHOTINO_LINUX_LOG(
        "[linux-geometry] save normal: x=%d y=%d width=%d height=%d\n",
        x,
        y,
        width,
        height);
}

// Restore size only. Restoring toplevel position is not reliable on Wayland,
// because the compositor owns global window placement.
void Photino::RestoreNormalGeometry()
{
    if (!platform_->window || !platform_->hasNormalGeometry)
        return;

    const auto geometry = platform_->normalGeometry;

    PHOTINO_LINUX_LOG(
        "[linux-geometry] restore normal size: width=%d height=%d\n",
        geometry.width,
        geometry.height);

    gtk_window_resize(GTK_WINDOW(platform_->window), geometry.width, geometry.height);
    // Do not restore position on Linux. Some GTK backends/window managers do not expose reliable toplevel coordinates.
}

void Photino::ScheduleRestoreNormalGeometry()
{
    if (!platform_->restoreNormalGeometryAfterUnmaximize ||
        platform_->restoreNormalGeometryScheduled)
    {
        return;
    }

    platform_->restoreNormalGeometryScheduled = true;

    g_idle_add(restore_normal_geometry_idle, this);
    //g_timeout_add(100, restore_normal_geometry_idle, this);
}

void Photino::CompleteScheduledRestoreNormalGeometry()
{
    if (!platform_->window)
    {
        platform_->restoreNormalGeometryAfterUnmaximize = false;
        platform_->restoreNormalGeometryScheduled = false;
        return;
    }

    RestoreNormalGeometry();

    platform_->restoreNormalGeometryAfterUnmaximize = false;
    platform_->restoreNormalGeometryScheduled = false;

    UpdateWindowState();
}


bool Photino::Activate() const
{
    assert(platform_->window);
    if (!platform_->window) return false;

    gtk_window_present(GTK_WINDOW(platform_->window));
    return true;
}

bool Photino::Center() const
{
    assert(platform_->window);
    if (!platform_->window) return false;

    gint windowWidth = 0;
    gint windowHeight = 0;
    gtk_window_get_size(GTK_WINDOW(platform_->window), &windowWidth, &windowHeight);

    GdkDisplay* display = gdk_display_get_default();
    if (!display)
        return false;

    GdkWindow* gdkWindow = gtk_widget_get_window(platform_->window);
    GdkMonitor* gdkMonitor = gdkWindow
                              ? gdk_display_get_monitor_at_window(display, gdkWindow)
                              : gdk_display_get_primary_monitor(display);

    if (gdkMonitor == nullptr)
    {
        gdkMonitor = gdk_display_get_monitor(display, 0);
        if (gdkMonitor == nullptr)
            return false;
    }

    GdkRectangle screen{};
    gdk_monitor_get_workarea(gdkMonitor, &screen);

    gtk_window_move(
        GTK_WINDOW(platform_->window),
        screen.x + (screen.width - windowWidth) / 2,
        screen.y + (screen.height - windowHeight) / 2);

    return true;
}

// Wayland does not expose reliable global coordinates for toplevel windows.
// Configure event x/y may stay at 0,0 and should be treated as best-effort.
void Photino::HandleConfigureEvent(int x, int y, int width, int height)
{
    PHOTINO_LINUX_LOG(
        "[linux-handle] HandleConfigureEvent: x=%d y=%d width=%d height=%d transitioning=%d exiting=%d pending=%d\n",
        x,
        y,
        width,
        height,
        platform_->isFullScreenTransitioning,
        platform_->isExitingFullScreen,
        static_cast<int>(platform_->pendingStateAfterFullScreenExit));

    if (!platform_->isFullScreenTransitioning &&
        !platform_->restoreNormalGeometryAfterUnmaximize &&
        GetPlatformWindowState() == PhotinoWindowState::Normal)
    {
        SaveNormalGeometry();
    }

    if (!platform_->isFullScreenTransitioning)
        UpdateWindowState();

    if (platform_->lastGeometry.left != x || platform_->lastGeometry.top != y)
    {
        // Configure x/y are best-effort for toplevel windows.
        // On Wayland they are not reliable global screen coordinates.
        InvokeMove(x, y);
        platform_->lastGeometry.left = x;
        platform_->lastGeometry.top = y;
    }

    if (platform_->lastGeometry.width != width || platform_->lastGeometry.height != height)
    {
        InvokeResize(width, height);
        platform_->lastGeometry.width = width;
        platform_->lastGeometry.height = height;
    }
}

void Photino::HandleWindowStateEvent()
{
    TraceLinuxState("HandleWindowStateEvent:entry", this);

    if (!platform_->window)
        return;

    GdkWindow* gdkWindow = gtk_widget_get_window(GTK_WIDGET(platform_->window));
    if (!gdkWindow)
        return;

    const GdkWindowState state = gdk_window_get_state(gdkWindow);

    PHOTINO_LINUX_LOG(
        "[linux-handle] HandleWindowStateEvent: transitioning=%d exiting=%d pending=%d logicalMinimized=%d raw=0x%x\n",
        platform_->isFullScreenTransitioning,
        platform_->isExitingFullScreen,
        static_cast<int>(platform_->pendingStateAfterFullScreenExit),
        platform_->logicalMinimized,
        static_cast<unsigned int>(state));

    if (!platform_->isFullScreenTransitioning)
    {
        if (platform_->logicalMinimized && (state & GDK_WINDOW_STATE_FOCUSED))
        {
            PHOTINO_LINUX_LOG("[linux-handle] HandleWindowStateEvent: clearing logicalMinimized on focus\n");
            platform_->logicalMinimized = false;
        }

        if (platform_->restoreNormalGeometryAfterUnmaximize &&
            (state & GDK_WINDOW_STATE_MAXIMIZED) == 0)
        {
            ScheduleRestoreNormalGeometry();
        }

        TraceLinuxState("HandleWindowStateEvent:before-update", this);
        UpdateWindowState();
        TraceLinuxState("HandleWindowStateEvent:after-update", this);
        return;
    }

    if (!platform_->isExitingFullScreen)
    {
        if (state & GDK_WINDOW_STATE_FULLSCREEN)
        {
            PHOTINO_LINUX_LOG("[linux-handle] HandleWindowStateEvent: fullscreen entered\n");

            platform_->isFullScreenTransitioning = false;

            TraceLinuxState("HandleWindowStateEvent:before-enter-update", this);
            UpdateWindowState();
            TraceLinuxState("HandleWindowStateEvent:after-enter-update", this);
        }

        return;
    }

    if (state & GDK_WINDOW_STATE_FULLSCREEN)
    {
        PHOTINO_LINUX_LOG("[linux-handle] HandleWindowStateEvent: still fullscreen, waiting\n");
        return;
    }

    const auto pendingState = platform_->pendingStateAfterFullScreenExit;

    PHOTINO_LINUX_LOG(
        "[linux-handle] HandleWindowStateEvent: fullscreen exited, applying pending=%d\n",
        static_cast<int>(pendingState));

    GtkWindow* window = GTK_WINDOW(platform_->window);

    switch (pendingState)
    {
    case PhotinoWindowState::Maximized:
        if ((state & GDK_WINDOW_STATE_MAXIMIZED) == 0)
        {
            PHOTINO_LINUX_LOG("[linux-handle] applying pending Maximized\n");
            gtk_window_maximize(window);
            return;
        }

        PHOTINO_LINUX_LOG("[linux-handle] pending Maximized completed\n");
        CompleteFullScreenTransition();
        return;

    case PhotinoWindowState::Minimized:
        if ((state & GDK_WINDOW_STATE_ICONIFIED) == 0)
        {
            PHOTINO_LINUX_LOG("[linux-handle] applying pending Minimized via logical fallback\n");
            gtk_window_iconify(window);
            platform_->logicalMinimized = true;
        }

        PHOTINO_LINUX_LOG("[linux-handle] pending Minimized completed\n");
        CompleteFullScreenTransition();
        return;

    case PhotinoWindowState::Normal:
    default:
        if (state & GDK_WINDOW_STATE_MAXIMIZED)
        {
            PHOTINO_LINUX_LOG("[linux-handle] applying pending Normal by unmaximizing\n");
            gtk_window_unmaximize(window);
            return;
        }

        PHOTINO_LINUX_LOG("[linux-handle] pending Normal completed\n");
        CompleteFullScreenTransition();
        return;
    }
}

void Photino::CompleteFullScreenTransition()
{
    TraceLinuxState("CompleteFullScreenTransition:before", this);

    PHOTINO_LINUX_LOG(
        "[linux-transition] CompleteFullScreenTransition: pending=%d transitioning=%d exiting=%d logicalMinimized=%d\n",
        static_cast<int>(platform_->pendingStateAfterFullScreenExit),
        platform_->isFullScreenTransitioning,
        platform_->isExitingFullScreen,
        platform_->logicalMinimized);

    platform_->pendingStateAfterFullScreenExit = PhotinoWindowState::Normal;
    platform_->isFullScreenTransitioning = false;
    platform_->isExitingFullScreen = false;

    TraceLinuxState("CompleteFullScreenTransition:before-update", this);
    UpdateWindowState();
    TraceLinuxState("CompleteFullScreenTransition:after-update", this);
}

bool Photino::Maximize()
{
    PHOTINO_LINUX_LOG(
        "[linux-command] Maximize: fullscreen=%d maximized=%d minimized=%d transitioning=%d exiting=%d pending=%d\n",
        IsFullScreen(),
        IsMaximized(),
        IsMinimized(),
        platform_->isFullScreenTransitioning,
        platform_->isExitingFullScreen,
        static_cast<int>(platform_->pendingStateAfterFullScreenExit));

    assert(platform_->window);
    if (!platform_->window) return false;

    if (platform_->isFullScreenTransitioning)
    {
        if (platform_->isExitingFullScreen)
            platform_->pendingStateAfterFullScreenExit = PhotinoWindowState::Maximized;

        return true;
    }

    GtkWindow* window = GTK_WINDOW(platform_->window);

    if (IsMinimized())
    {
        gtk_window_deiconify(window);
        platform_->logicalMinimized = false;
    }

    if (IsFullScreen())
    {
        platform_->pendingStateAfterFullScreenExit = PhotinoWindowState::Maximized;
        platform_->isFullScreenTransitioning = true;
        platform_->isExitingFullScreen = true;

        gtk_window_unfullscreen(window);
        return true;
    }

    if (!IsMaximized())
    {
        if (GetPlatformWindowState() == PhotinoWindowState::Normal)
            SaveNormalGeometry();

        gtk_window_maximize(window);
    }

    UpdateWindowState();

    return true;
}

bool Photino::Minimize()
{
    PHOTINO_LINUX_LOG(
        "[linux-command] Minimize: fullscreen=%d maximized=%d minimized=%d transitioning=%d exiting=%d pending=%d\n",
        IsFullScreen(),
        IsMaximized(),
        IsMinimized(),
        platform_->isFullScreenTransitioning,
        platform_->isExitingFullScreen,
        static_cast<int>(platform_->pendingStateAfterFullScreenExit));

    assert(platform_->window);
    if (!platform_->window) return false;

    if (platform_->isFullScreenTransitioning)
    {
        if (platform_->isExitingFullScreen)
            platform_->pendingStateAfterFullScreenExit = PhotinoWindowState::Minimized;

        return true;
    }

    GtkWindow* window = GTK_WINDOW(platform_->window);

    if (IsFullScreen())
    {
        platform_->pendingStateAfterFullScreenExit = PhotinoWindowState::Minimized;
        platform_->isFullScreenTransitioning = true;
        platform_->isExitingFullScreen = true;

        gtk_window_unfullscreen(window);
        return true;
    }

    if (!IsMinimized())
    {
        gtk_window_iconify(window);
        platform_->logicalMinimized = true;
    }

    UpdateWindowState();

    return true;
}

bool Photino::Restore()
{
    PHOTINO_LINUX_LOG(
        "[linux-command] Restore: fullscreen=%d maximized=%d minimized=%d transitioning=%d exiting=%d pending=%d logicalMinimized=%d\n",
        IsFullScreen(),
        IsMaximized(),
        IsMinimized(),
        platform_->isFullScreenTransitioning,
        platform_->isExitingFullScreen,
        static_cast<int>(platform_->pendingStateAfterFullScreenExit),
        platform_->logicalMinimized);

    assert(platform_->window);
    if (!platform_->window) return false;

    if (platform_->isFullScreenTransitioning)
    {
        // If fullscreen is already exiting, explicit Restore should target Normal.
        // But only during an already-running exit transition.
        if (platform_->isExitingFullScreen)
            platform_->pendingStateAfterFullScreenExit = PhotinoWindowState::Normal;

        TraceLinuxState("Restore:ignored-during-transition", this);
        return true;
    }

    GtkWindow* window = GTK_WINDOW(platform_->window);

    if (IsFullScreen())
    {
        PHOTINO_LINUX_LOG("[linux-command] Restore: exiting fullscreen\n");

        platform_->isFullScreenTransitioning = true;
        platform_->isExitingFullScreen = true;

        // Do not overwrite pendingStateAfterFullScreenExit here.
        // It contains the state that should be restored after fullscreen exit.

        gtk_window_unfullscreen(window);

        TraceLinuxState("Restore:after-unfullscreen-request", this);
        return true;
    }

    const bool wasMinimized = IsMinimized();

    if (wasMinimized)
    {
        PHOTINO_LINUX_LOG("[linux-command] Restore: deiconify\n");

        gtk_window_deiconify(window);
        platform_->logicalMinimized = false;

        gtk_window_present(window);

        TraceLinuxState("Restore:after-deiconify-request", this);
    }

    bool requestedUnmaximize = false;

    if (!wasMinimized && IsMaximized())
    {
        PHOTINO_LINUX_LOG("[linux-command] Restore: unmaximize\n");

        platform_->restoreNormalGeometryAfterUnmaximize = platform_->hasNormalGeometry;

        TraceLinuxState("Restore:before-unmaximize", this);
        gtk_window_unmaximize(window);
        TraceLinuxState("Restore:after-unmaximize-request", this);

        requestedUnmaximize = true;
    }

    if (requestedUnmaximize)
        return true;

    TraceLinuxState("Restore:before-update", this);
    UpdateWindowState();
    TraceLinuxState("Restore:after-update", this);

    return true;
}

bool Photino::Show() const
{
    assert(platform_->window);
    if (!platform_->window) return false;

    gtk_widget_show_all(platform_->window);
    return true;
}

void Photino::UpdateWebViewInputShape() const noexcept
{
    if (!platform_->webview)
        return;

    if (!options_.chromeless || !CanBeginResize())
    {
        gtk_widget_input_shape_combine_region(platform_->webview, nullptr);
        return;
    }

    const int border = platform_->chromelessSettings.ResizeBorderThickness;
    const int width = gtk_widget_get_allocated_width(platform_->webview);
    const int height = gtk_widget_get_allocated_height(platform_->webview);

    int effectiveBorder = border;
    effectiveBorder = (std::min)(effectiveBorder, (std::max)(0, (width - 1) / 2));
    effectiveBorder = (std::min)(effectiveBorder, (std::max)(0, (height - 1) / 2));

    if (effectiveBorder <= 0)
    {
        gtk_widget_input_shape_combine_region(platform_->webview, nullptr);
        return;
    }

    cairo_rectangle_int_t rect =
        {
            effectiveBorder,
            effectiveBorder,
            width - 2 * effectiveBorder,
            height - 2 * effectiveBorder};

    auto region = cairo_region_create_rectangle(&rect);
    gtk_widget_input_shape_combine_region(platform_->webview, region);
    cairo_region_destroy(region);
}

bool Photino::CanBeginResize() const noexcept
{
    return platform_->window &&
           options_.chromeless &&
           options_.resizable &&
           platform_->chromelessSettings.ResizeBorderThickness > 0 &&
           !platform_->isFullScreenTransitioning &&
           !IsFullScreen() &&
           !IsMaximized();
}

bool Photino::CanBeginDrag() const noexcept
{
    return platform_->window &&
           options_.chromeless &&
           platform_->chromelessSettings.DragRegionHeight > 0 &&
           !platform_->isFullScreenTransitioning &&
           !IsFullScreen() &&
           !IsMaximized() &&
           !IsMinimized();
}

void Photino::BeginWindowDrag() const
{
    // Linux chromeless drag is started from native GDK button events over the
    // configured drag region. Wayland requires that originating trusted button
    // event context, so this generic WebView-message entry point remains a no-op.
}

void Photino::BeginWindowResize(PhotinoWindowEdge) const
{
    // Linux resize is handled from native GTK edge button events. This generic
    // entry point has no originating GdkEventButton context and remains a no-op.
}

unsigned int Photino::GetScreenDpi() const
{
    if (!platform_->window) return 96;

    GdkScreen* screen = gtk_window_get_screen(GTK_WINDOW(platform_->window));
    if (!screen) return 96;

    gdouble dpi = gdk_screen_get_resolution(screen);
    return dpi < 0 ? 96 : static_cast<unsigned int>(dpi);
}

bool Photino::GetAllMonitors(GetAllMonitorsCallback callback, void* state) const noexcept
{
    assert(callback);
    if (!callback) return false;

    assert(platform_->window);
    if (!platform_->window) return false;

    GdkScreen* screen = gtk_window_get_screen(GTK_WINDOW(platform_->window));
    if (!screen) return false;

    GdkDisplay* display = gdk_screen_get_display(screen);
    if (!display) return false;

    int n = gdk_display_get_n_monitors(display);
    for (int i = 0; i < n; i++)
    {
        GdkMonitor* gdkMonitor = gdk_display_get_monitor(display, i);
        if (!gdkMonitor) continue;

        Monitor props{};
        if (!TryGetMonitorInfo(gdkMonitor, props))
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

    GdkDisplay* display = gdk_display_get_default();
    if (!display)
        return false;

    GdkWindow* gdkWindow = gtk_widget_get_window(platform_->window);
    GdkMonitor* gdkMonitor = gdkWindow
                                 ? gdk_display_get_monitor_at_window(display, gdkWindow)
                                 : gdk_display_get_primary_monitor(display);

    if (!gdkMonitor)
        return false;

    return TryGetMonitorInfo(gdkMonitor, monitor);
}

bool Photino::IsFullScreen() const noexcept
{
    if (!platform_->window)
        return false;

    GdkWindow* gdkWindow = gtk_widget_get_window(GTK_WIDGET(platform_->window));
    if (!gdkWindow)
        return false;

    return (gdk_window_get_state(gdkWindow) & GDK_WINDOW_STATE_FULLSCREEN) != 0;
}

bool Photino::IsMinimized() const noexcept
{
    if (!platform_->window)
        return false;

    if (platform_->logicalMinimized)
        return true;

    GdkWindow* gdkWindow = gtk_widget_get_window(GTK_WIDGET(platform_->window));
    if (!gdkWindow)
        return false;

    return (gdk_window_get_state(gdkWindow) & GDK_WINDOW_STATE_ICONIFIED) != 0;
}

bool Photino::IsMaximized() const noexcept
{
    if (!platform_->window)
        return false;

    GdkWindow* gdkWindow = gtk_widget_get_window(GTK_WIDGET(platform_->window));
    if (!gdkWindow)
        return false;

    return (gdk_window_get_state(gdkWindow) & GDK_WINDOW_STATE_MAXIMIZED) != 0;
}

PhotinoWindowState Photino::GetPlatformWindowState() const noexcept
{
    if (!platform_->window)
        return options_.windowState;

    GdkWindow* gdkWindow = gtk_widget_get_window(GTK_WIDGET(platform_->window));
    if (!gdkWindow)
        return options_.windowState;

    const GdkWindowState state = gdk_window_get_state(gdkWindow);

    if (platform_->logicalMinimized || (state & GDK_WINDOW_STATE_ICONIFIED))
        return PhotinoWindowState::Minimized;

    if (state & GDK_WINDOW_STATE_FULLSCREEN)
        return PhotinoWindowState::FullScreen;

    if (platform_->isFullScreenTransitioning &&
        platform_->isExitingFullScreen &&
        platform_->pendingStateAfterFullScreenExit != PhotinoWindowState::Normal)
    {
        return platform_->pendingStateAfterFullScreenExit;
    }

    if (state & GDK_WINDOW_STATE_MAXIMIZED)
        return PhotinoWindowState::Maximized;

    return PhotinoWindowState::Normal;
}

void Photino::SetFullScreen(bool fullScreen)
{
    PHOTINO_LINUX_LOG("[linux-command] SetFullScreen(%d)\n", fullScreen);

    assert(platform_->window);
    if (!platform_->window) return;

    GtkWindow* window = GTK_WINDOW(platform_->window);

    const bool isFullScreen = IsFullScreen();

    if (isFullScreen == fullScreen)
    {
        UpdateWindowState();
        return;
    }

    if (fullScreen)
    {
        if (GetPlatformWindowState() == PhotinoWindowState::Normal)
            SaveNormalGeometry();

        // Minimized is a logical state here: logicalMinimized may hide a raw
        // GDK_WINDOW_STATE_MAXIMIZED state underneath. Preserve that raw maximized
        // state so Maximized -> Minimized -> FullScreen -> Restore returns Maximized.
        const bool wasMinimized = IsMinimized();
        const bool wasMaximized = IsMaximized();

        if (wasMinimized)
        {
            gtk_window_deiconify(window);
            platform_->logicalMinimized = false;
            platform_->pendingStateAfterFullScreenExit = wasMaximized 
                ? PhotinoWindowState::Maximized 
                : PhotinoWindowState::Normal;

            gtk_window_present(window);
        }
        else if (wasMaximized)
        {
            platform_->pendingStateAfterFullScreenExit = PhotinoWindowState::Maximized;
        }
        else
        {
            platform_->pendingStateAfterFullScreenExit = PhotinoWindowState::Normal;
        }

        platform_->isFullScreenTransitioning = true;
        platform_->isExitingFullScreen = false;

        gtk_window_fullscreen(window);

        if (wasMinimized)
            gtk_window_present(window);

        return;
    }

    platform_->isFullScreenTransitioning = true;
    platform_->isExitingFullScreen = true;

    gtk_window_unfullscreen(window);
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

    if (IsFullScreen() || platform_->isFullScreenTransitioning)
    {
        UpdateWindowState();
        return;
    }

    if (IsMaximized())
    {
        platform_->restoreNormalGeometryAfterUnmaximize = platform_->hasNormalGeometry;
        gtk_window_unmaximize(GTK_WINDOW(platform_->window));
    }

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

    if (IsFullScreen() || platform_->isFullScreenTransitioning)
    {
        UpdateWindowState();
        return;
    }

    if (IsMinimized())
    {
        GtkWindow* window = GTK_WINDOW(platform_->window);

        gtk_window_deiconify(window);
        platform_->logicalMinimized = false;

        gtk_window_present(window);
    }

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

    gtk_window_set_resizable(GTK_WINDOW(platform_->window), resizable);

    UpdateWebViewInputShape();

    if (!resizable)
    {
        auto gdkWindow = gtk_widget_get_window(platform_->window);
        if (gdkWindow)
            gdk_window_set_cursor(gdkWindow, nullptr);
    }
}

void Photino::GetTopmost(bool* topmost) const
{
    assert(topmost);
    if (!topmost) return;

    *topmost = false;

    if (!platform_->window) return;

    // TODO: This flag is not set in GDK3. WebKit does not support GTK5 yet.
    GdkWindow* gdkWindow = gtk_widget_get_window(GTK_WIDGET(platform_->window));
    if (!gdkWindow) return;

    GdkWindowState flags = gdk_window_get_state(gdkWindow);
    *topmost = (flags & GDK_WINDOW_STATE_ABOVE) != 0;
}

void Photino::SetTopmost(bool topmost)
{
    assert(platform_->window);
    if (!platform_->window) return;

    gtk_window_set_keep_above(GTK_WINDOW(platform_->window), topmost);
}