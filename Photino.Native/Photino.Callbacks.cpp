#include "Photino.h"
#include "Photino.Enums.h"
#include "Photino.Strings.h"

using namespace PhotinoX::Native;

bool Photino::InvokeClosing() const noexcept
{
    if (!closingCallback_ || isClosing_)
        return false;

    isClosing_ = true;
    bool result = closingCallback_(callbackState_);
    isClosing_ = false;

    return result; // Closing: true = cancel close, false = allow close
}

void Photino::InvokeClose() const noexcept
{
    if (closedCallback_) closedCallback_(callbackState_);
}

void Photino::InvokeFocusIn() const noexcept
{
    if (focusInCallback_) focusInCallback_(callbackState_);
}

void Photino::InvokeFocusOut() const noexcept
{
    if (focusOutCallback_) focusOutCallback_(callbackState_);
}

void Photino::InvokeResize(const int width, const int height) const noexcept
{
    if (resizedCallback_) resizedCallback_(width, height, callbackState_);
}

void Photino::InvokeMove(const int x, const int y) const noexcept
{
    if (movedCallback_) movedCallback_(x, y, callbackState_);
}

void Photino::InvokeMaximized() const noexcept
{
    if (maximizedCallback_) maximizedCallback_(callbackState_);
}

void Photino::InvokeRestored() const noexcept
{
    if (restoredCallback_) restoredCallback_(callbackState_);
}

void Photino::InvokeMinimized() const noexcept
{
    if (minimizedCallback_) minimizedCallback_(callbackState_);
}

void Photino::InvokeFullScreenChanged(bool fullScreen) const noexcept
{
    if (fullScreenChangedCallback_) fullScreenChangedCallback_(fullScreen, callbackState_);
}

void Photino::InvokeStateChanged(PhotinoWindowState oldState, PhotinoWindowState newState) const noexcept
{
    if (stateChangedCallback_) stateChangedCallback_(oldState, newState, callbackState_);
}

void Photino::InvokeWebMessageReceived(const PlatformString& message, const PlatformString& uri) const noexcept
{
    if (!webMessageReceivedCallback_)
        return;

    std::string utf8Message = ToUtf8String(message);
    std::string utf8Uri = ToUtf8String(uri);

    webMessageReceivedCallback_(utf8Message.c_str(), utf8Uri.c_str(), callbackState_);
}

void* Photino::InvokeCustomScheme(Utf8String url, int* numBytes, Utf8String* contentType) const noexcept
{
    if (!customSchemeCallback_)
        return nullptr;

    return customSchemeCallback_(url, numBytes, contentType, callbackState_);
}

bool Photino::InvokeNavigationStarting(const PlatformString& uri) const noexcept
{
    if (!navigationStartingCallback_)
        return false;

    std::string utf8Uri = ToUtf8String(uri);
    return navigationStartingCallback_(utf8Uri.c_str(), callbackState_); // NavigationStarting: true = cancel navigation, false = allow/default behavior
}

bool Photino::InvokeNewWindowRequested(const PlatformString& uri) const noexcept
{
    if (!newWindowRequestedCallback_)
        return false;

    std::string utf8Uri = ToUtf8String(uri);
    return newWindowRequestedCallback_(utf8Uri.c_str(), callbackState_); // Reserved: NewWindowRequested: true = handled, false = allow/default behavior
}

void Photino::InvokeContentLoading(const PlatformString& uri) const noexcept
{
    if (!contentLoadingCallback_)
        return;

    std::string utf8Uri = ToUtf8String(uri);
    contentLoadingCallback_(utf8Uri.c_str(), callbackState_);
}

void Photino::InvokeContentLoaded(const PlatformString& uri) const noexcept
{
    if (!contentLoadedCallback_)
        return;

    std::string utf8Uri = ToUtf8String(uri);
    contentLoadedCallback_(utf8Uri.c_str(), callbackState_);
}