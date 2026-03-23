#pragma once

#include <windows.h>

namespace overlay
{
    class TrayIcon
    {
    public:
        bool Create(HWND hwnd, UINT callback_message, UINT icon_resource_id, const wchar_t* tooltip);
        void Remove();

    private:
        NOTIFYICONDATA notify_icon_data_ = {};
        bool created_ = false;
    };
}
