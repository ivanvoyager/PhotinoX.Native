#pragma once

namespace PhotinoX::Native
{
    struct Monitor
    {
        struct MonitorRect
        {
            int x, y;
            int width, height;
        } monitor, work;
        double scale;
    };
}