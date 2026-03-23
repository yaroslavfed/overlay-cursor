#include "tray_icon.h"

namespace overlay
{
    bool TrayIcon::Create(HWND hwnd, UINT callback_message, UINT icon_resource_id, const wchar_t* tooltip)
    {
        notify_icon_data_ = {};
        notify_icon_data_.cbSize = sizeof(NOTIFYICONDATA);
        notify_icon_data_.hWnd = hwnd;
        notify_icon_data_.uID = 1;
        notify_icon_data_.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
        notify_icon_data_.uCallbackMessage = callback_message;
        notify_icon_data_.hIcon = reinterpret_cast<HICON>(LoadImage(
            GetModuleHandle(nullptr),
            MAKEINTRESOURCE(icon_resource_id),
            IMAGE_ICON,
            16,
            16,
            LR_DEFAULTCOLOR));

        wcscpy_s(notify_icon_data_.szTip, tooltip);

        created_ = (Shell_NotifyIcon(NIM_ADD, &notify_icon_data_) == TRUE);
        return created_;
    }

    void TrayIcon::Remove()
    {
        if (!created_)
        {
            return;
        }

        Shell_NotifyIcon(NIM_DELETE, &notify_icon_data_);
        DestroyIcon(notify_icon_data_.hIcon);
        notify_icon_data_ = {};
        created_ = false;
    }
}
