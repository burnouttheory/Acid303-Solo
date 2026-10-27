#pragma once
#include "Sequencer.h"
#include <array>
#include <cstdint>
#include <random>
#include <algorithm>

namespace acid303::seq {
// Repeatable bass-line generator. Called only on UI thread; the audio thread
// reads each of the 16 steps through the processor's existing atomics.
inline std::array<Step, 16> generateAcidPattern(std::uint32_t seed) {
    std::mt19937 rng(seed);
    auto roll = [&rng](int limit) { return static_cast<int>(rng() % static_cast<std::uint32_t>(limit)); };
    constexpr int minor[] = {0, 3, 5, 7, 10, 12, 15, 17, 19};
    const int root = 36 + 12 * roll(2); // C2/C3, minor scale
    std::array<Step, 16> output{};
    int previous = root;
    int active = 0;
    for (int i = 0; i < 16; ++i) {
        auto& step = output[static_cast<std::size_t>(i)];
        const bool strong = (i % 4 == 0);
        step.on = (roll(100) < (strong ? 90 : 64));
        if (strong && i == 0) step.on = true;
        int best = root + minor[roll(9)];
        // Encourage recognisable TB-303 phrases rather than wide, random jumps.
        for (int attempt = 0; attempt < 4; ++attempt) {
            const int candidate = root + minor[roll(9)];
            if (std::abs(candidate - previous) <= 7) { best = candidate; break; }
        }
        step.note = static_cast<std::uint8_t>(std::clamp(best, 0, 127));
        step.accent = step.on && (roll(100) < (strong ? 52 : 29));
        step.slide = step.on && i > 0 && output[static_cast<std::size_t>(i - 1)].on
                     && (roll(100) < 28);
        if (step.on) { previous = best; ++active; }
    }
    // Musically useful minimum density; no all-rest or near-silent patterns.
    for (int i = 0; active < 6 && i < 16; ++i) {
        if (!output[static_cast<std::size_t>(i)].on && (i % 2 == 0 || active < 3)) {
            output[static_cast<std::size_t>(i)].on = true;
            output[static_cast<std::size_t>(i)].slide = false;
            ++active;
        }
    }
    return output;
}
} // namespace acid303::seq
