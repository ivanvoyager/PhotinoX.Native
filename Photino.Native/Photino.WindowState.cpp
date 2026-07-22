#include "Photino.h"
#include "Photino.Enums.h"

#include <cassert>

using namespace PhotinoX::Native;

void Photino::GetFullScreen(bool* fullScreen) const
{
    assert(fullScreen);
    if (!fullScreen) return;

    *fullScreen = GetPlatformWindowState() == WindowState::FullScreen;
}

void Photino::GetMaximized(bool* isMaximized) const
{
    assert(isMaximized);
    if (!isMaximized) return;

    *isMaximized = GetPlatformWindowState() == WindowState::Maximized;
}

void Photino::GetMinimized(bool* isMinimized) const
{
    assert(isMinimized);
    if (!isMinimized) return;

    *isMinimized = GetPlatformWindowState() == WindowState::Minimized;
}

void Photino::GetWindowState(WindowState* state) const
{
    assert(state);
    if (!state) return;

    *state = GetPlatformWindowState();
}

void Photino::SetWindowState(const WindowState state)
{
    switch (state)
    {
    case WindowState::Normal:
        Restore();
        break;
    case WindowState::Minimized:
        Minimize();
        break;
    case WindowState::Maximized:
        Maximize();
        break;
    case WindowState::FullScreen:
        SetFullScreen(true);
        break;
    default:
        assert(false);
        break;
    }
}

bool Photino::ChangeWindowState(WindowState state) noexcept
{
    const auto oldState = options_.windowState;

    if (oldState == state)
        return false;

    options_.windowState = state;
    InvokeStateChanged(oldState, state);

    return true;
}

bool Photino::UpdateWindowState() noexcept
{
    const auto oldState = options_.windowState;
    const auto newState = GetPlatformWindowState();

    if (oldState == newState)
        return false;

    if (suppressWindowStateCallbacks_)
    {
        options_.windowState = newState;
        return true;
    }

    if (!ChangeWindowState(newState))
        return false;

    const bool wasFullScreen = oldState == WindowState::FullScreen;
    const bool isFullScreen = newState == WindowState::FullScreen;

    if (wasFullScreen != isFullScreen)
        InvokeFullScreenChanged(isFullScreen);

    switch (newState)
    {
    case WindowState::Maximized:
        InvokeMaximized();
        break;
    case WindowState::Minimized:
        InvokeMinimized();
        break;
    case WindowState::Normal:
        if (!suppressRestoredCallback_)
            InvokeRestored();
        break;
    default:
        break;
    }

    return true;
}