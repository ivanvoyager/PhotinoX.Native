#include "Photino.h"
#include "Photino.Enums.h"

#include <cassert>

using namespace PhotinoX::Native;

void Photino::GetFullScreen(bool* fullScreen) const
{
    assert(fullScreen);
    if (!fullScreen) return;

    *fullScreen = GetPlatformWindowState() == PhotinoWindowState::FullScreen;
}

void Photino::GetMaximized(bool* isMaximized) const
{
    assert(isMaximized);
    if (!isMaximized) return;

    *isMaximized = GetPlatformWindowState() == PhotinoWindowState::Maximized;
}

void Photino::GetMinimized(bool* isMinimized) const
{
    assert(isMinimized);
    if (!isMinimized) return;

    *isMinimized = GetPlatformWindowState() == PhotinoWindowState::Minimized;
}

void Photino::GetWindowState(PhotinoWindowState* state) const
{
    assert(state);
    if (!state) return;

    *state = GetPlatformWindowState();
}

void Photino::SetWindowState(const PhotinoWindowState state)
{
    switch (state)
    {
    case PhotinoWindowState::Normal:
        Restore();
        break;
    case PhotinoWindowState::Minimized:
        Minimize();
        break;
    case PhotinoWindowState::Maximized:
        Maximize();
        break;
    case PhotinoWindowState::FullScreen:
        SetFullScreen(true);
        break;
    default:
        assert(false);
        break;
    }
}

bool Photino::ChangeWindowState(PhotinoWindowState state) noexcept
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

    const bool wasFullScreen = oldState == PhotinoWindowState::FullScreen;
    const bool isFullScreen = newState == PhotinoWindowState::FullScreen;

    if (wasFullScreen != isFullScreen)
        InvokeFullScreenChanged(isFullScreen);

    switch (newState)
    {
    case PhotinoWindowState::Maximized:
        InvokeMaximized();
        break;
    case PhotinoWindowState::Minimized:
        InvokeMinimized();
        break;
    case PhotinoWindowState::Normal:
        if (!suppressRestoredCallback_)
            InvokeRestored();
        break;
    default:
        break;
    }

    return true;
}