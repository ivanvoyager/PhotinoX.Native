#include "Photino.h"

using namespace PhotinoX::Native;

bool Photino::InvokeClosing() const noexcept
{
    if (!_closingCallback || _isClosing)
        return false;

    _isClosing = true;
    bool result = _closingCallback();
    _isClosing = false;

    return result;
}

void Photino::InvokeClose() const noexcept
{
    if (_closedCallback) _closedCallback();
}

void Photino::InvokeFocusIn() const noexcept
{
    if (_focusInCallback) _focusInCallback();
}

void Photino::InvokeFocusOut() const noexcept
{
    if (_focusOutCallback) _focusOutCallback();
}

void Photino::InvokeMove(int x, int y) const noexcept
{
    if (_movedCallback) _movedCallback(x, y);
}

void Photino::InvokeResize(int width, int height) const noexcept
{
    if (_resizedCallback) _resizedCallback(width, height);
}

void Photino::InvokeMaximized() const noexcept
{
    if (_maximizedCallback) _maximizedCallback();
}

void Photino::InvokeRestored() const noexcept
{
    if (_restoredCallback) _restoredCallback();
}

void Photino::InvokeMinimized() const noexcept
{
    if (_minimizedCallback) _minimizedCallback();
}