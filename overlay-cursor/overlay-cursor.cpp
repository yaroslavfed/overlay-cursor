#include <windows.h>

LPCWSTR getLayoutText() {
    HWND activeWnd = GetForegroundWindow();
    DWORD threadId = GetWindowThreadProcessId(activeWnd, NULL);
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

    // Создаём overlay
    HWND hwnd = CreateWindowEx(
        WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE | WS_EX_TRANSPARENT,
        CLASS_NAME,
        L"Overlay",
        WS_POPUP,
        0, 0, 50, 30,
        NULL, NULL, hInstance, NULL
    );

    ShowWindow(hwnd, nCmdShow);

    MSG msg = {};

    while (true) {
        // Обработка сообщений
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) return 0;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        // Получаем позицию курсора
        POINT pt;
        GetCursorPos(&pt);

        // Создаём offscreen DC и bitmap
        HDC screenDC = GetDC(NULL);
        HDC memDC = CreateCompatibleDC(screenDC);
        HBITMAP hBitmap = CreateCompatibleBitmap(screenDC, 50, 30);
        HGDIOBJ oldBmp = SelectObject(memDC, hBitmap);

        // Чистим bitmap (прозрачный фон)
        PatBlt(memDC, 0, 0, 50, 30, BLACKNESS);

        // Шрифт
        HFONT hFont = CreateFontW(
            12, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
            CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY,
            DEFAULT_PITCH | FF_SWISS, L"Segoe UI"
        );
        HGDIOBJ oldFont = SelectObject(memDC, hFont);

        SetBkMode(memDC, TRANSPARENT);

        LPCWSTR text = getLayoutText();
        if (text[0] == 'E') SetTextColor(memDC, RGB(0, 255, 0));    // EN — зелёный
        else SetTextColor(memDC, RGB(0, 128, 255));              // RU — синий

        TextOutW(memDC, 0, 0, text, lstrlenW(text));

        // Обновляем layered window
        SIZE size = { 50,30 };
        POINT zero = { 0,0 };
        BLENDFUNCTION blend = { AC_SRC_OVER,0,255,AC_SRC_ALPHA };
        POINT pos = { pt.x + 12, pt.y + 12 }; // смещение от курсора
        UpdateLayeredWindow(hwnd, screenDC, &pos, &size, memDC, &zero, RGB(0, 0, 0), &blend, ULW_ALPHA);

        // Очистка
        SelectObject(memDC, oldFont);
        SelectObject(memDC, oldBmp);
        DeleteObject(hFont);
        DeleteObject(hBitmap);
        DeleteDC(memDC);
        ReleaseDC(NULL, screenDC);

        Sleep(16); // ~60 FPS
    }
}