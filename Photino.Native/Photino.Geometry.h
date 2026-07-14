#pragma once

namespace PhotinoX::Native {
    struct WindowGeometry {
        int left = 0;
        int top = 0;
        int width = 0;
        int height = 0;
    };

    struct WindowSizeLimits {
        int minWidth = 0;
        int minHeight = 0;
        int maxWidth = 0;
        int maxHeight = 0;
    };

} // namespace PhotinoX::Native