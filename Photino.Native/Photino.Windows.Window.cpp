#include "Photino.h"

#include <Windows.h>

#include <cassert>

using namespace PhotinoX::Native;

void Photino::Close() const
{
    assert(_hWnd);
    if (!_hWnd) return;

    BOOL result = PostMessageW(_hWnd, WM_CLOSE, 0, 0);
    assert(result);
}