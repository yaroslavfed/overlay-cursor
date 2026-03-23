#include "overlay_renderer.h"

#include <cstring>

#include "app_constants.h"

namespace overlay
{
    COLORREF OverlayRenderer::LerpColor(COLORREF from, COLORREF to, float t)
    {
        BYTE r = static_cast<BYTE>(GetRValue(from) + (GetRValue(to) - GetRValue(from)) * t);
        BYTE g = static_cast<BYTE>(GetGValue(from) + (GetGValue(to) - GetGValue(from)) * t);
        BYTE b = static_cast<BYTE>(GetBValue(from) + (GetBValue(to) - GetBValue(from)) * t);
        return RGB(r, g, b);
    }
    
    void OverlayRenderer::DrawTextWithOutline(HDC dc, int x, int y, LPCWSTR text, COLORREF color)
    {
        int len = lstrlenW(text);

        SetTextColor(dc, RGB(0, 0, 0));
        for (int dx = -2; dx <= 2; dx++)
        {
            for (int dy = -2; dy <= 2; dy++)
            {
                if (dx == 0 && dy == 0) continue;
                TextOutW(dc, x + dx, y + dy, text, len);
            }
        }

        SetTextColor(dc, color);
        TextOutW(dc, x, y, text, len);
    }

    bool OverlayRenderer::Initialize()
    {
        screen_dc_ = GetDC(nullptr);
        if (!screen_dc_)
        {
            return false;
        }

        mem_dc_ = CreateCompatibleDC(screen_dc_);
        if (!mem_dc_)
        {
            ReleaseDC(nullptr, screen_dc_);
            screen_dc_ = nullptr;
            return false;
        }

        BITMAPINFO bmi = {};
        bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth = kOverlayWidth;
        bmi.bmiHeader.biHeight = -kOverlayHeight;
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 32;
        bmi.bmiHeader.biCompression = BI_RGB;

        bitmap_ = CreateDIBSection(mem_dc_, &bmi, DIB_RGB_COLORS, &bits_, nullptr, 0);
        if (!bitmap_ || !bits_)
        {
            DeleteDC(mem_dc_);
            ReleaseDC(nullptr, screen_dc_);
            mem_dc_ = nullptr;
            screen_dc_ = nullptr;
            return false;
        }
        old_bitmap_ = SelectObject(mem_dc_, bitmap_);

        font_ = CreateFontW(
            12,
            0,
            0,
            0,
            FW_BOLD,
            FALSE,
            FALSE,
            FALSE,
            DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS,
            CLIP_DEFAULT_PRECIS,
            NONANTIALIASED_QUALITY,
            DEFAULT_PITCH | FF_SWISS,
            L"Segoe UI");

        if (!font_)
        {
            SelectObject(mem_dc_, old_bitmap_);
            DeleteObject(bitmap_);
            DeleteDC(mem_dc_);
            ReleaseDC(nullptr, screen_dc_);
            bitmap_ = nullptr;
            mem_dc_ = nullptr;
            screen_dc_ = nullptr;
            return false;
        }
        old_font_ = SelectObject(mem_dc_, font_);
        SetBkMode(mem_dc_, TRANSPARENT);

        size_ = {kOverlayWidth, kOverlayHeight};
        zero_ = {0, 0};
        blend_ = {AC_SRC_OVER, 0, 255, AC_SRC_ALPHA};

        initialized_ = true;
        return true;
    }

    void OverlayRenderer::Shutdown()
    {
        if (!initialized_)
        {
            return;
        }

        SelectObject(mem_dc_, old_font_);
        SelectObject(mem_dc_, old_bitmap_);
        DeleteObject(font_);
        DeleteObject(bitmap_);
        DeleteDC(mem_dc_);
        ReleaseDC(nullptr, screen_dc_);

        screen_dc_ = nullptr;
        mem_dc_ = nullptr;
        bitmap_ = nullptr;
        old_bitmap_ = nullptr;
        font_ = nullptr;
        old_font_ = nullptr;
        bits_ = nullptr;
        initialized_ = false;
    }

    bool OverlayRenderer::RenderFrame(HWND hwnd, bool force)
    {
        LayoutId layout = DetectActiveLayout();
        bool layout_changed = false;
        if (layout != last_layout_)
        {
            last_layout_ = layout;
            last_text_ = GetLayoutText(layout);
            target_color_ = GetLayoutColor(layout);
            layout_changed = true;
            force = true;
        }

        COLORREF next_color = LerpColor(current_color_, target_color_, kColorLerpStep);
        bool color_changed = (next_color != current_color_);
        current_color_ = next_color;

        POINT cursor_pos = {};
        GetCursorPos(&cursor_pos);
        POINT overlay_pos = {cursor_pos.x + kOverlayCursorOffsetPx, cursor_pos.y + kOverlayCursorOffsetPx};
        bool position_changed = (overlay_pos.x != last_pos_.x || overlay_pos.y != last_pos_.y);
        last_pos_ = overlay_pos;

        if (!force && !position_changed && !color_changed)
        {
            return false;
        }

        std::memset(bits_, 0, kOverlayWidth * kOverlayHeight * 4);
        constexpr int kTextPadding = 2;

        DrawTextWithOutline(mem_dc_, kTextPadding, kTextPadding, last_text_, current_color_);
        UpdateLayeredWindow(hwnd, screen_dc_, &overlay_pos, &size_, mem_dc_, &zero_, RGB(0, 0, 0), &blend_, ULW_ALPHA);

        return layout_changed || position_changed || color_changed;
    }
}
