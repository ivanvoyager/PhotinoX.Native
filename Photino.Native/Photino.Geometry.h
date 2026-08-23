#pragma once

#include "Photino.Enums.h"

#include <algorithm>
#include <type_traits>
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

    struct Rect
    {
        int x, y;
        int width, height;
    };
    static_assert(std::is_standard_layout_v<Rect>,
                  "Rect must remain standard-layout for managed/native interop.");
    static_assert(sizeof(Rect) == 16,
                  "Rect size changed. Update the managed ABI layout and size validation.");

    struct Monitor
    {
        Rect monitor;
        Rect work;
        double scale;
    };
    static_assert(std::is_standard_layout_v<Monitor>,
                  "Monitor must remain standard-layout for managed/native interop.");
    static_assert(sizeof(Monitor) == 40,
                  "Monitor size changed. Update the managed ABI layout and size validation.");

    struct Thickness
    {
        int left = 0;
        int top = 0;
        int right = 0;
        int bottom = 0;
    };
    static_assert(std::is_standard_layout_v<Thickness>,
                  "Thickness must remain standard-layout for managed/native interop.");
    static_assert(sizeof(Thickness) == 16,
                  "Thickness size changed. Update the managed ABI layout and size validation.");

    struct LayoutRegion
    {
        int width = 0;
        int height = 0;

        Thickness margin;

        HorizontalAlignment horizontalAlignment = HorizontalAlignment::Left;
        VerticalAlignment verticalAlignment = VerticalAlignment::Top;
    };
    static_assert(std::is_standard_layout_v<LayoutRegion>,
                  "LayoutRegion must remain standard-layout for managed/native interop.");
    static_assert(sizeof(LayoutRegion) == 32,
                  "LayoutRegion size changed. Update the managed ABI layout and size validation.");

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