#include <windows.h>
#include <cstring>
#include <shellapi.h>

#include "Resource.h"

#define WM_TRAYICON (WM_APP + 1)
#define ID_TRAY_ABOUT 1001
#define ID_TRAY_EXIT  1002
#define ID_RENDER_TIMER 1

NOTIFYICONDATA nid = {};

namespace
{
    constexpr wchar_t kClassName[] = L"OverlayWindow";
    constexpr int kWidth = 50;
    constexpr int kHeight = 30;
    constexpr int kOffsetPx = 12;
    constexpr int kActiveFrameDelayMs = 33; // ~30 FPS: плавность при движении/анимации
    constexpr int kIdleFrameDelayMs = 120;  // в простое снижаем частоту опроса
    constexpr float kColorLerpStep = 0.2f;

    enum class LayoutId
    {
        En,
        Ru,
        Unknown
    };

    struct RenderContext
    {
        HDC screenDC = nullptr;
        HDC memDC = nullptr;
        HBITMAP hBitmap = nullptr;
        HGDIOBJ oldBmp = nullptr;
        HFONT hFont = nullptr;
        HGDIOBJ oldFont = nullptr;
        void* bits = nullptr;
        SIZE size = {kWidth, kHeight};
        POINT zero = {0, 0};
        BLENDFUNCTION blend = {AC_SRC_OVER, 0, 255, AC_SRC_ALPHA};
        bool initialized = false;
    };

    RenderContext gRender;
    LayoutId gLastLayout = LayoutId::Unknown;
    LPCWSTR gLastText = L"??";
    COLORREF gCurrentColor = RGB(0, 0, 0);
    COLORREF gTargetColor = RGB(180, 180, 180);
    POINT gLastPos = {-1, -1};
    int gCurrentTimerDelayMs = kActiveFrameDelayMs;
}

LayoutId getLayoutId()
{
    HWND foreground = GetForegroundWindow();
    if (!foreground)
    {
        return LayoutId::Unknown;
    }

    DWORD threadId = GetWindowThreadProcessId(foreground, NULL);
    HKL layout = GetKeyboardLayout(threadId);
    WORD lang = LOWORD(reinterpret_cast<UINT_PTR>(layout));

    if (lang == 0x419) return LayoutId::Ru;
    if (lang == 0x409) return LayoutId::En;
    return LayoutId::Unknown;
}

LPCWSTR getLayoutText(LayoutId layoutId)
{
    switch (layoutId)
    {
    case LayoutId::En: return L"EN";
    case LayoutId::Ru: return L"RU";
    default: return L"??";
    }
}

COLORREF getTargetColor(LayoutId layoutId)
{
    switch (layoutId)
    {
    case LayoutId::En: return RGB(0, 255, 0);
    case LayoutId::Ru: return RGB(0, 128, 255);
    default: return RGB(180, 180, 180);
    }
}

COLORREF lerpColor(COLORREF from, COLORREF to, float t)
{
    BYTE r = static_cast<BYTE>(GetRValue(from) + (GetRValue(to) - GetRValue(from)) * t);
    BYTE g = static_cast<BYTE>(GetGValue(from) + (GetGValue(to) - GetGValue(from)) * t);
    BYTE b = static_cast<BYTE>(GetBValue(from) + (GetBValue(to) - GetBValue(from)) * t);
    return RGB(r, g, b);
}

bool initRenderResources()
{
    gRender.screenDC = GetDC(NULL);
    if (!gRender.screenDC)
    {
        return false;
    }

    gRender.memDC = CreateCompatibleDC(gRender.screenDC);
    if (!gRender.memDC)
    {
        ReleaseDC(NULL, gRender.screenDC);
        gRender.screenDC = nullptr;
        return false;
    }

    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = kWidth;
    bmi.bmiHeader.biHeight = -kHeight;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    gRender.hBitmap = CreateDIBSection(gRender.memDC, &bmi, DIB_RGB_COLORS, &gRender.bits, NULL, 0);
    if (!gRender.hBitmap || !gRender.bits)
    {
        DeleteDC(gRender.memDC);
        ReleaseDC(NULL, gRender.screenDC);
        gRender.memDC = nullptr;
        gRender.screenDC = nullptr;
        return false;
    }
    gRender.oldBmp = SelectObject(gRender.memDC, gRender.hBitmap);

    gRender.hFont = CreateFontW(
        12, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_SWISS, L"Segoe UI"
    );
    if (!gRender.hFont)
    {
        SelectObject(gRender.memDC, gRender.oldBmp);
        DeleteObject(gRender.hBitmap);
        DeleteDC(gRender.memDC);
        ReleaseDC(NULL, gRender.screenDC);
        gRender.hBitmap = nullptr;
        gRender.memDC = nullptr;
        gRender.screenDC = nullptr;
        return false;
    }
    gRender.oldFont = SelectObject(gRender.memDC, gRender.hFont);
    SetBkMode(gRender.memDC, TRANSPARENT);

    gRender.initialized = true;
    return true;
}

void cleanupRenderResources()
{
    if (!gRender.initialized)
    {
        return;
    }

    SelectObject(gRender.memDC, gRender.oldFont);
    SelectObject(gRender.memDC, gRender.oldBmp);
    DeleteObject(gRender.hFont);
    DeleteObject(gRender.hBitmap);
    DeleteDC(gRender.memDC);
    ReleaseDC(NULL, gRender.screenDC);

    gRender = {};
}

void updateTimerInterval(HWND hwnd, int delayMs)
{
    if (gCurrentTimerDelayMs == delayMs)
    {
        return;
    }

    SetTimer(hwnd, ID_RENDER_TIMER, delayMs, NULL);
    gCurrentTimerDelayMs = delayMs;
}

bool renderOverlay(HWND hwnd, bool force)
{
    LayoutId layoutId = getLayoutId();
    LPCWSTR text = getLayoutText(layoutId);
    bool layoutChanged = false;
    if (layoutId != gLastLayout)
    {
        gLastLayout = layoutId;
        gLastText = text;
        gTargetColor = getTargetColor(layoutId);
        layoutChanged = true;
        force = true;
    }

    COLORREF nextColor = lerpColor(gCurrentColor, gTargetColor, kColorLerpStep);
    bool colorChanged = (nextColor != gCurrentColor);
    gCurrentColor = nextColor;

    POINT pt;
    GetCursorPos(&pt);
    POINT pos = {pt.x + kOffsetPx, pt.y + kOffsetPx};
    bool positionChanged = (pos.x != gLastPos.x || pos.y != gLastPos.y);
    gLastPos = pos;

    if (!force && !positionChanged && !colorChanged)
    {
        return false;
    }

    std::memset(gRender.bits, 0, kWidth * kHeight * 4);
    SetTextColor(gRender.memDC, gCurrentColor);
    TextOutW(gRender.memDC, 0, 0, gLastText, lstrlenW(gLastText));
    UpdateLayeredWindow(hwnd, gRender.screenDC, &pos, &gRender.size, gRender.memDC, &gRender.zero, RGB(0, 0, 0), &gRender.blend, ULW_ALPHA);
    return layoutChanged || positionChanged || colorChanged;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CREATE:
        {
            nid.cbSize = sizeof(NOTIFYICONDATA);
            nid.hWnd = hwnd;
            nid.uID = 1;
            nid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
            nid.uCallbackMessage = WM_TRAYICON;
            nid.hIcon = (HICON)LoadImage(
                GetModuleHandle(NULL),
                MAKEINTRESOURCE(IDI_APP_ICON),
                IMAGE_ICON,
                16, 16,
                LR_DEFAULTCOLOR
            );
            wcscpy_s(nid.szTip, L"Keyboard Layout Overlay");

            Shell_NotifyIcon(NIM_ADD, &nid);
            break;
        }

    case WM_TRAYICON:
        if (lParam == WM_RBUTTONUP)
        {
            POINT pt;
            GetCursorPos(&pt);

            HMENU menu = CreatePopupMenu();
            AppendMenu(menu, MF_STRING, ID_TRAY_ABOUT, L"About");
            AppendMenu(menu, MF_SEPARATOR, 0, NULL);
            AppendMenu(menu, MF_STRING, ID_TRAY_EXIT, L"Quit");

            SetForegroundWindow(hwnd);
            TrackPopupMenu(menu, TPM_BOTTOMALIGN | TPM_LEFTALIGN, pt.x, pt.y, 0, hwnd, NULL);
            DestroyMenu(menu);
        }
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case ID_TRAY_ABOUT:
            MessageBox(hwnd, L"Keyboard Layout Overlay\nVersion 1.0", L"About", MB_OK);
            break;

        case ID_TRAY_EXIT:
            DestroyWindow(hwnd);
            break;
        }
        break;

    case WM_TIMER:
        if (wParam == ID_RENDER_TIMER && gRender.initialized)
        {
            bool isActive = renderOverlay(hwnd, false);
            updateTimerInterval(hwnd, isActive ? kActiveFrameDelayMs : kIdleFrameDelayMs);
        }
        break;

    case WM_DESTROY:
        KillTimer(hwnd, ID_RENDER_TIMER);
        Shell_NotifyIcon(NIM_DELETE, &nid);
        DestroyIcon(nid.hIcon);
        cleanupRenderResources();
        PostQuitMessage(0);
        break;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
    WNDCLASS wc = {};
    wc.lpfnWndProc = WndProc;
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

    if (!initRenderResources())
    {
        DestroyWindow(hwnd);
        return 1;
    }

    SetTimer(hwnd, ID_RENDER_TIMER, kActiveFrameDelayMs, NULL);
    ShowWindow(hwnd, nCmdShow);
    renderOverlay(hwnd, true);

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}
