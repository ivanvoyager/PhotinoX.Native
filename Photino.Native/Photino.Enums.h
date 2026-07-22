#pragma once

namespace PhotinoX::Native
{
    // Which edge or corner a resize drag operates on. The ordering is the ABI
    // contract with the managed PhotinoWindowEdge enum and must stay in sync.
    enum class PhotinoWindowEdge : int
    {
        Top,
        Bottom,
        Left,
        Right,
        TopLeft,
        TopRight,
        BottomLeft,
        BottomRight,
    };

    // Native window state. The ordering is the ABI contract with the managed
    // PhotinoWindowState enum and must stay in sync.
    enum class PhotinoWindowState : int
    {
        Normal,
        Minimized,
        Maximized,
        FullScreen,
    };
}