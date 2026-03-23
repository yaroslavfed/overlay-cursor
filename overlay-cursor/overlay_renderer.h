#pragma once

#include <windows.h>

#include "keyboard_layout.h"

namespace overlay
{
    class OverlayRenderer
    {
    public:
        OverlayRenderer() = default;
        ~OverlayRenderer() = default;

        OverlayRenderer(const OverlayRenderer&) = delete;
        OverlayRenderer& operator=(const OverlayRenderer&) = delete;

        bool Initialize();
        void Shutdown();

        // Renders one frame if state changed.
        // Returns true when app should stay in active timer cadence.
        bool RenderFrame(HWND hwnd, bool force);

    private:
        static COLORREF LerpColor(COLORREF from, COLORREF to, float t);

        HDC screen_dc_ = nullptr;
        HDC mem_dc_ = nullptr;
        HBITMAP bitmap_ = nullptr;
        HGDIOBJ old_bitmap_ = nullptr;
        HFONT font_ = nullptr;
        HGDIOBJ old_font_ = nullptr;
        void* bits_ = nullptr;

        SIZE size_ = {};
        POINT zero_ = {};
        BLENDFUNCTION blend_ = {};

        LayoutId last_layout_ = LayoutId::Unknown;
        LPCWSTR last_text_ = L"??";
        COLORREF current_color_ = RGB(0, 0, 0);
        COLORREF target_color_ = RGB(180, 180, 180);
        POINT last_pos_ = {-1, -1};

        bool initialized_ = false;
    };
}
