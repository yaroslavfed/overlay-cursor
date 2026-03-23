#include "keyboard_layout.h"

namespace overlay
{
    LayoutId DetectActiveLayout()
    {
        HWND foregroundWindow = GetForegroundWindow();
        if (!foregroundWindow)
        {
            return LayoutId::Unknown;
        }

        DWORD threadId = GetWindowThreadProcessId(foregroundWindow, nullptr);
        HKL keyboardLayout = GetKeyboardLayout(threadId);
        WORD languageId = LOWORD(reinterpret_cast<UINT_PTR>(keyboardLayout));

        if (languageId == 0x419)
        {
            return LayoutId::Ru;
        }

        if (languageId == 0x409)
        {
            return LayoutId::En;
        }

        return LayoutId::Unknown;
    }

    LPCWSTR GetLayoutText(LayoutId layoutId)
    {
        switch (layoutId)
        {
        case LayoutId::En:
            return L"EN";
        case LayoutId::Ru:
            return L"RU";
        default:
            return L"??";
        }
    }

    COLORREF GetLayoutColor(LayoutId layoutId)
    {
        switch (layoutId)
        {
        case LayoutId::En:
            return RGB(199,252,236);
        case LayoutId::Ru:
            return RGB(255,192,203);
        default:
            return RGB(120, 120, 120);
        }
    }
}
