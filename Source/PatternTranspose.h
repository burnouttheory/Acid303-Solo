#pragma once
// Independent, testable non-destructive semitone transposition.
#include <algorithm>
namespace acid303::transpose {
inline constexpr int midiPitch(int storedNote, int semitones) {
    return std::clamp(storedNote + std::clamp(semitones,-12,12),0,127);
}
} // namespace acid303::transpose
