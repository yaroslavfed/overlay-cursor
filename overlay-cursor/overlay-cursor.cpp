#include <windows.h>

LPCWSTR getLayoutText() {
    DWORD threadId = GetWindowThreadProcessId(GetForegroundWindow(), NULL);
    HKL layout = GetKeyboardLayout(threadId);
    int lang = LOWORD(layout);

    if (lang == 0x419) return L"RU";
    if (lang == 0x409) return L"EN";
    return L"??";
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    const wchar_t CLASS_NAME[] = L"OverlayWindow";

    // Регистрация окна
    WNDCLASS wc = {};
    wc.lpfnWndProc = DefWindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    RegisterClass(&wc);

    const int width = 50, height = 30;
    HWND hwnd = CreateWindowEx(
        WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE | WS_EX_TRANSPARENT,
        CLASS_NAME,
        L"Overlay",
        WS_POPUP,
        0, 0, width, height,
        NULL, NULL, hInstance, NULL
    );
    ShowWindow(hwnd, nCmdShow);

    // 32-битный bitmap с альфа-каналом
    HDC screenDC = GetDC(NULL);
    HDC memDC = CreateCompatibleDC(screenDC);

    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biHeight = -height; // верхний левый угол = (0,0)
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    void* bits = nullptr;
    HBITMAP hBitmap = CreateDIBSection(memDC, &bmi, DIB_RGB_COLORS, &bits, NULL, 0);
    HGDIOBJ oldBmp = SelectObject(memDC, hBitmap);

    HFONT hFont = CreateFontW(
        12, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_SWISS, L"Segoe UI"
    );
    HGDIOBJ oldFont = SelectObject(memDC, hFont);
    SetBkMode(memDC, TRANSPARENT);

    LPCWSTR lastText = nullptr;

    auto drawText = [&]() {
        memset(bits, 0, width * height * 4); // прозрачный фон

        LPCWSTR text = getLayoutText();
        if (text[0] == L'E') SetTextColor(memDC, RGB(0, 255, 0));
        else SetTextColor(memDC, RGB(0, 128, 255));

        TextOutW(memDC, 0, 0, text, lstrlenW(text));
        lastText = text;
        };

    drawText(); // первый раз

    MSG msg = {};
    SIZE size = { width, height };
    POINT zero = { 0,0 };
    BLENDFUNCTION blend = { AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };

    while (true) {
        // Обработка сообщений
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) break;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        LPCWSTR text = getLayoutText();
        if (text != lastText) drawText();

        // Мгновенная позиция overlay у курсора
        POINT pt;
        GetCursorPos(&pt);
        POINT pos = { pt.x + 12, pt.y + 12 }; // смещение от курсора

        UpdateLayeredWindow(hwnd, screenDC, &pos, &size, memDC, &zero, RGB(0, 0, 0), &blend, ULW_ALPHA);

        Sleep(16); // ~60 FPS
    }

    // Очистка
    SelectObject(memDC, oldFont);
    SelectObject(memDC, oldBmp);
    DeleteObject(hFont);
    DeleteObject(hBitmap);
    DeleteDC(memDC);
    ReleaseDC(NULL, screenDC);
}