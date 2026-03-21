# overlay-cursor

Минималистичный Win32 overlay, который показывает активную раскладку клавиатуры рядом с курсором.

## Текущая архитектура файлов

```text
overlay-cursor.sln
overlay-cursor/
  overlay-cursor.vcxproj
  overlay-cursor.cpp        # основной runtime (WinMain, render loop, WinAPI drawing)
  overlay-cursor.rc         # ресурсы приложения (иконки/метаданные)
  overlay-cursor.h          # заголовок приложения (precompiled include chain)
  framework.h               # стандартный include wrapper из шаблона VS
  targetver.h               # версия Windows SDK
  Resource.h                # resource IDs
  overlay-cursor.ico
  small.ico
```

## Оценка архитектуры

Для текущего масштаба (один исполняемый файл, одна ключевая функция) структура **корректная и рабочая**:

- проектные и ресурсные файлы отделены от кода;
- вход в приложение и render loop находятся в одном месте;
- зависимости ограничены WinAPI (без лишних библиотек).

## Когда стоит реструктурировать

Если планируется рост функциональности (настройки, трэй-меню, hotkeys, разные темы, несколько overlay-виджетов), лучше заранее разделить код:

```text
overlay-cursor/
  src/
    app_main.cpp
    overlay_window.cpp
    keyboard_layout.cpp
    color_animator.cpp
  include/
    overlay_window.h
    keyboard_layout.h
    color_animator.h
  resources/
    overlay-cursor.rc
    Resource.h
    icons/...
```

Это уменьшит связность, упростит тестирование логики и ускорит поддержку при добавлении фич.
