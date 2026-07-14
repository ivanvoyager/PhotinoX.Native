#include "Photino.h"
#include "Photino.Mac.Internal.h"

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