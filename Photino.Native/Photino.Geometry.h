#pragma once

#include "Photino.Enums.h"

#include <algorithm>
#include <vector>

namespace PhotinoX::Native
{
    struct WindowGeometry
    {
        int left = 0;
        int top = 0;
        int width = 0;
        int height = 0;
    };

    struct WindowSizeLimits
    {
        int minWidth = 0;
        int minHeight = 0;
        int maxWidth = 0;
        int maxHeight = 0;
    };

    struct Thickness
    {
        int left = 0;
        int top = 0;
        int right = 0;
        int bottom = 0;
    };

    struct LayoutRegion
    {
        int width = 0;
        int height = 0;

        Thickness margin;

        HorizontalAlignment horizontalAlignment = HorizontalAlignment::Left;
        VerticalAlignment verticalAlignment = VerticalAlignment::Top;
    };

    inline bool IsInLayoutRange(
        int position,
        int availableLength,
        int length,
        int startMargin,
        int endMargin,
        HorizontalAlignment alignment)
    {
        const int availableRegionLength = (std::max)(
            0,
            availableLength - startMargin - endMargin);

        int start;
        int resolvedLength;

        switch (alignment)
        {
        case HorizontalAlignment::Left:
            start = startMargin;
            resolvedLength = length;
            break;

        case HorizontalAlignment::Center:
            start = startMargin + (availableRegionLength - length) / 2;
            resolvedLength = length;
            break;

        case HorizontalAlignment::Right:
            start = availableLength - endMargin - length;
            resolvedLength = length;
            break;

        case HorizontalAlignment::Stretch:
            start = startMargin;
            resolvedLength = availableRegionLength;
            break;

        default:
            return false;
        }

        resolvedLength = (std::max)(0, resolvedLength);

        return position >= start && position < start + resolvedLength;
    }

    inline bool IsInLayoutRange(
        int position,
        int availableLength,
        int length,
        int startMargin,
        int endMargin,
        VerticalAlignment alignment)
    {
        const int availableRegionLength = (std::max)(
            0,
            availableLength - startMargin - endMargin);

        int start;
        int resolvedLength;

        switch (alignment)
        {
        case VerticalAlignment::Top:
            start = startMargin;
            resolvedLength = length;
            break;

        case VerticalAlignment::Center:
            start = startMargin + (availableRegionLength - length) / 2;
            resolvedLength = length;
            break;

        case VerticalAlignment::Bottom:
            start = availableLength - endMargin - length;
            resolvedLength = length;
            break;

        case VerticalAlignment::Stretch:
            start = startMargin;
            resolvedLength = availableRegionLength;
            break;

        default:
            return false;
        }

        resolvedLength = (std::max)(0, resolvedLength);

        return position >= start && position < start + resolvedLength;
    }

    inline bool IsInLayoutRegion(
        const LayoutRegion& region,
        int x,
        int y,
        int availableWidth,
        int availableHeight)
    {
        return IsInLayoutRange(
                   x,
                   availableWidth,
                   region.width,
                   region.margin.left,
                   region.margin.right,
                   region.horizontalAlignment) &&
               IsInLayoutRange(
                   y,
                   availableHeight,
                   region.height,
                   region.margin.top,
                   region.margin.bottom,
                   region.verticalAlignment);
    }

    inline bool IsInAnyLayoutRegion(
        const std::vector<LayoutRegion>& regions,
        int x,
        int y,
        int availableWidth,
        int availableHeight)
    {
        for (const auto& region : regions)
        {
            if (IsInLayoutRegion(region, x, y, availableWidth, availableHeight))
                return true;
        }

        return false;
    }

} // namespace PhotinoX::Native