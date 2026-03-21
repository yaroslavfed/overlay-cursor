#include <windows.h>
#include <cstring>

namespace {
constexpr wchar_t kClassName[] = L"OverlayWindow";
constexpr int kWidth = 50;
constexpr int kHeight = 30;
constexpr int kOffsetPx = 12;
constexpr int kFrameDelayMs = 16; // ~60 FPS

enum class LayoutId {
    En,
    Ru,
    Unknown
};
}

LayoutId getLayoutId() {
    HWND foreground = GetForegroundWindow();
    if (!foreground) {
        return LayoutId::Unknown;
    }

    DWORD threadId = GetWindowThreadProcessId(foreground, NULL);
    HKL layout = GetKeyboardLayout(threadId);
    WORD lang = LOWORD(reinterpret_cast<UINT_PTR>(layout));

    if (lang == 0x419) return LayoutId::Ru;
    if (lang == 0x409) return LayoutId::En;
    return LayoutId::Unknown;
}

LPCWSTR getLayoutText(LayoutId layoutId) {
    switch (layoutId) {
    case LayoutId::En: return L"EN";
    case LayoutId::Ru: return L"RU";
    default: return L"??";
    }
}

COLORREF getTargetColor(LayoutId layoutId) {
    switch (layoutId) {
    case LayoutId::En: return RGB(0, 255, 0);
    case LayoutId::Ru: return RGB(0, 128, 255);
    default: return RGB(180, 180, 180);
    }
}

COLORREF lerpColor(COLORREF from, COLORREF to, float t) {
    BYTE r = static_cast<BYTE>(GetRValue(from) + (GetRValue(to) - GetRValue(from)) * t);
    BYTE g = static_cast<BYTE>(GetGValue(from) + (GetGValue(to) - GetGValue(from)) * t);
    BYTE b = static_cast<BYTE>(GetBValue(from) + (GetBValue(to) - GetBValue(from)) * t);
    return RGB(r, g, b);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    WNDCLASS wc = {};
    wc.lpfnWndProc = DefWindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = kClassName;
    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(
        WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE | WS_EX_TRANSPARENT,
        kClassName,
        L"Overlay",
        WS_POPUP,
        0, 0, kWidth, kHeight,
        NULL, NULL, hInstance, NULL
    );
    if (!hwnd) return 1;
    ShowWindow(hwnd, nCmdShow);

    // 32-битный bitmap с альфа-каналом
    HDC screenDC = GetDC(NULL);
    HDC memDC = CreateCompatibleDC(screenDC);
    if (!screenDC || !memDC) return 1;

    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = kWidth;
    bmi.bmiHeader.biHeight = -kHeight;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    void* bits = nullptr;
    HBITMAP hBitmap = CreateDIBSection(memDC, &bmi, DIB_RGB_COLORS, &bits, NULL, 0);
    if (!hBitmap || !bits) return 1;
    HGDIOBJ oldBmp = SelectObject(memDC, hBitmap);

    HFONT hFont = CreateFontW(
        12, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_SWISS, L"Segoe UI"
    );
    if (!hFont) return 1;
    HGDIOBJ oldFont = SelectObject(memDC, hFont);
    SetBkMode(memDC, TRANSPARENT);

    LayoutId lastLayout = LayoutId::Unknown;
    LPCWSTR lastText = getLayoutText(lastLayout);
    COLORREF currentColor = RGB(0, 0, 0);
    COLORREF targetColor = getTargetColor(lastLayout);

    MSG msg = {};
    bool running = true;
    SIZE size = { kWidth, kHeight };
    POINT zero = { 0,0 };
    BLENDFUNCTION blend = { AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };

    while (running) {
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                running = false;
                break;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        LayoutId layoutId = getLayoutId();
        LPCWSTR text = getLayoutText(layoutId);
        // если язык изменился — меняем целевой цвет
        if (layoutId != lastLayout) {
            lastLayout = layoutId;
            lastText = text;
            targetColor = getTargetColor(layoutId);
        }

        // плавная интерполяция цвета (fade)
        currentColor = lerpColor(currentColor, targetColor, 0.2f);

        // рисуем текст на прозрачном фоне
        std::memset(bits, 0, kWidth * kHeight * 4);
        SetTextColor(memDC, currentColor);
        TextOutW(memDC, 0, 0, lastText, lstrlenW(lastText));

        // мгновенная позиция overlay у курсора
        POINT pt;
        GetCursorPos(&pt);
        POINT pos = { pt.x + kOffsetPx, pt.y + kOffsetPx };

        UpdateLayeredWindow(hwnd, screenDC, &pos, &size, memDC, &zero, RGB(0, 0, 0), &blend, ULW_ALPHA);

        Sleep(kFrameDelayMs);
    }

    SelectObject(memDC, oldFont);
    SelectObject(memDC, oldBmp);
    DeleteObject(hFont);
    DeleteObject(hBitmap);
    DeleteDC(memDC);
    ReleaseDC(NULL, screenDC);
    return 0;
}
