# Overlay Cursor

A small Windows utility that shows your current keyboard layout (`EN`, `RU`, `??`) right next to the cursor.

You can:
- just download the `.exe` and use it;
- or dive into the code and tweak it however you like.

---

## 🚀 Quick Start

1. Download `overlay-cursor.exe` from the release.
2. Run it (if Windows warns you — just confirm).
3. Done:
   - an icon will appear in the system tray;
   - the layout indicator will show near your cursor.

To exit: right-click the tray icon → **Quit**.

💡 If you use it often, consider adding it to startup.

---

## ✨ Features

- shows keyboard layout near the cursor;
- detects layout from the active window;
- smooth color transition on layout change;
- low CPU usage (has an idle mode).

---

## 🛠 Build from Source

**Requirements:**
- Windows 10/11  
- Visual Studio 2022 (C++ workload)

**Steps:**
1. Open `overlay-cursor.sln`
2. Select `Release + x64`
3. Click Build  
4. The `.exe` will appear in the `bin/` folder

---

## 📁 Project Structure

- `overlay-cursor.cpp` — entry point, window, loop
- `overlay_renderer.*` — rendering
- `keyboard_layout.*` — layout detection
- `tray_icon.*` — system tray
- `app_constants.h` — config/constants

---

## 🔧 Common Tweaks

- `EN/RU` colors
- indicator size & position
- update frequency
- font and text style

Check: `app_constants.h` and `overlay_renderer.cpp`

---

## ⚠️ Limitations

- only `EN` and `RU` are properly handled (`??` for others)
- Windows only (WinAPI/GDI)

---

## 💬 Feedback

Want more features (extra languages, settings, auto-start, UI customization)?  
Feel free to open an issue or PR 👍
