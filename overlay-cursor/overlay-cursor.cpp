#include <windows.h>

#include "Resource.h"
#include "app_constants.h"
#include "overlay_renderer.h"
#include "tray_icon.h"

namespace
{
    constexpr UINT kMsgTrayIcon = WM_APP + 1;
    constexpr UINT_PTR kRenderTimerId = 1;

    constexpr UINT kMenuAboutId = 1001;
    constexpr UINT kMenuExitId = 1002;

    overlay::OverlayRenderer g_renderer;
    overlay::TrayIcon g_tray_icon;
    int g_current_timer_delay_ms = overlay::kActiveFrameDelayMs;

    void UpdateTimerInterval(HWND hwnd, int delay_ms)
    {
        if (g_current_timer_delay_ms == delay_ms)
        {
            return;
        }

        SetTimer(hwnd, kRenderTimerId, delay_ms, nullptr);
        g_current_timer_delay_ms = delay_ms;
    }

    void ShowTrayMenu(HWND hwnd)
    {
        POINT cursor = {};
        GetCursorPos(&cursor);

        HMENU menu = CreatePopupMenu();
        AppendMenu(menu, MF_STRING, kMenuAboutId, L"About");
        AppendMenu(menu, MF_SEPARATOR, 0, nullptr);
        AppendMenu(menu, MF_STRING, kMenuExitId, L"Quit");

        SetForegroundWindow(hwnd);
        TrackPopupMenu(menu, TPM_BOTTOMALIGN | TPM_LEFTALIGN, cursor.x, cursor.y, 0, hwnd, nullptr);
        DestroyMenu(menu);
    }
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CREATE:
    {
        g_tray_icon.Create(hwnd, kMsgTrayIcon, IDI_APP_ICON, L"Keyboard Layout Overlay");
        return 0;
    }

    case kMsgTrayIcon:
        if (lParam == WM_RBUTTONUP)
        {
            ShowTrayMenu(hwnd);
        }
        return 0;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case kMenuAboutId:
            MessageBox(hwnd, L"Keyboard Layout Overlay\nVersion 1.0", L"About", MB_OK);
            return 0;

        case kMenuExitId:
            DestroyWindow(hwnd);
            return 0;

        default:
            break;
        }
        break;

    case WM_TIMER:
        if (wParam == kRenderTimerId)
        {
            const bool active = g_renderer.RenderFrame(hwnd, false);
            UpdateTimerInterval(hwnd, active ? overlay::kActiveFrameDelayMs : overlay::kIdleFrameDelayMs);
            return 0;
        }
        break;

    case WM_DESTROY:
        KillTimer(hwnd, kRenderTimerId);
        g_renderer.Shutdown();
        g_tray_icon.Remove();
        PostQuitMessage(0);
        return 0;

    default:
        break;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE instance, HINSTANCE, LPSTR, int cmd_show)
{
    WNDCLASS wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = instance;
    wc.lpszClassName = overlay::kWindowClassName;
    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(
        WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE | WS_EX_TRANSPARENT,
        overlay::kWindowClassName,
        L"Overlay",
        WS_POPUP,
        0,
        0,
        overlay::kOverlayWidth,
        overlay::kOverlayHeight,
        nullptr,
        nullptr,
        instance,
        nullptr);

    if (!hwnd)
    {
        return 1;
    }

    if (!g_renderer.Initialize())
    {
        DestroyWindow(hwnd);
        return 1;
    }

    SetTimer(hwnd, kRenderTimerId, overlay::kActiveFrameDelayMs, nullptr);
    ShowWindow(hwnd, cmd_show);
    g_renderer.RenderFrame(hwnd, true);

    MSG msg = {};
    while (GetMessage(&msg, nullptr, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}
