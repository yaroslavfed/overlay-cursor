#pragma once

namespace overlay
{
    // Window/class identifiers.
    constexpr wchar_t kWindowClassName[] = L"OverlayWindow";

    // Overlay geometry and behavior.
    constexpr int kOverlayWidth = 50;
    constexpr int kOverlayHeight = 30;
    constexpr int kOverlayCursorOffsetPx = 12;

    // Timer cadence.
    constexpr int kActiveFrameDelayMs = 33;  // ~30 FPS for smooth movement.
    constexpr int kIdleFrameDelayMs = 120;   // Lower polling rate while idle.

    // Color animation blend step (0..1).
    constexpr float kColorLerpStep = 0.2F;
}
