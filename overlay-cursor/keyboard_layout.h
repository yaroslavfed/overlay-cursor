#pragma once

#include <windows.h>

namespace overlay
{
    enum class LayoutId
    {
        En,
        Ru,
        Unknown
    };

    LayoutId DetectActiveLayout();
    LPCWSTR GetLayoutText(LayoutId layoutId);
    COLORREF GetLayoutColor(LayoutId layoutId);
}
