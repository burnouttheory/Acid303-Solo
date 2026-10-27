#pragma once
// JUCE-independent mapping used by the V3 VST3 processor and its unit tests.
#include <array>
#include <algorithm>
namespace acid303::bank {
constexpr int kGroups = 8;
constexpr int kPatternsPerGroup = 8;
constexpr int kTotalPatterns = kGroups * kPatternsPerGroup;
constexpr int kFirstMidi = 36;
constexpr int kLastMidi = kFirstMidi + kTotalPatterns - 1;
inline constexpr int slot(int group, int pattern) {
    return (group >= 0 && group < kGroups && pattern >= 0 && pattern < kPatternsPerGroup)
        ? group*kPatternsPerGroup+pattern : -1;
}
inline constexpr int fromMidi(int note) {
    return (note >= kFirstMidi && note <= kLastMidi) ? note - kFirstMidi : -1;
}
inline constexpr int toMidi(int index) {
    return (index >= 0 && index < kTotalPatterns) ? kFirstMidi+index : -1;
}
inline constexpr int group(int index) { return index>=0 && index<kTotalPatterns ? index/8 : -1; }
inline constexpr int pattern(int index) { return index>=0 && index<kTotalPatterns ? index%8 : -1; }
} // namespace acid303::bank
