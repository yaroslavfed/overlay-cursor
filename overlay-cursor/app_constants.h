#pragma once

namespace overlay
{
    // Window/class identifiers.
    inline constexpr wchar_t kWindowClassName[] = L"OverlayWindow";

    // Overlay geometry and behavior.
    inline constexpr int kOverlayWidth = 50;
    inline constexpr int kOverlayHeight = 30;
    inline constexpr int kOverlayCursorOffsetPx = 12;

    // Timer cadence.
    inline constexpr int kActiveFrameDelayMs = 33;  // ~30 FPS for smooth movement.
    inline constexpr int kIdleFrameDelayMs = 120;   // Lower polling rate while idle.

    // Color animation blend step (0..1).
    inline constexpr float kColorLerpStep = 0.2F;
}
