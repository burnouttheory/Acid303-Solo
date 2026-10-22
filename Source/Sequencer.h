#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>

// A one-bar (16 sixteenth notes) pattern. Entire representation is independent
// of JUCE so it can be tested and exchanged atomically with the audio thread.
namespace acid303::seq {
struct Step {
    bool on = false;
    uint8_t note = 48; // C3 in MIDI; the display octave depends on FL Studio preferences
    bool slide = false;  // glide INTO this step when previous step played
    bool accent = false; // louder velocity + stronger filter envelope
};
inline uint32_t pack(Step s) {
    return static_cast<uint32_t>(std::clamp<int>(s.note, 0, 127))
        | (s.on ? (1u << 8) : 0u)
        | (s.slide ? (1u << 9) : 0u)
        | (s.accent ? (1u << 10) : 0u);
}
inline Step unpack(uint32_t data) {
    return { (data & (1u << 8)) != 0u, static_cast<uint8_t>(std::min(127u, data & 127u)),
             (data & (1u << 9)) != 0u, (data & (1u << 10)) != 0u };
}
inline int64_t absoluteStep(double ppqPosition) {
    // One quarter-note beat has 4 sixteenths.
    return static_cast<int64_t>(std::floor(ppqPosition * 4.0 + 1.0e-9));
}
inline int patternIndex(int64_t absolute) {
    // Modulo adjusted for negative PPQ (pre-roll).
    return static_cast<int>((absolute % 16 + 16) % 16);
}
inline float accentVelocity(bool accented) { return accented ? 1.0f : 0.58f; }
} // namespace acid303::seq
