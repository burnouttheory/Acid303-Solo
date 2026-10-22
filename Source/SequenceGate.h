#pragma once
#include <array>
#include <algorithm>
#include <cmath>
#include <cstdint>

// MIDI note-gated pattern clock, independent of JUCE for unit tests.
// Start of note => step 1, release of last held note => no new steps.
// Clock divisions: quarter (1), eighth (2), sixteenth (4), thirty-second (8) steps / beat.
// Pattern length: 1..16 steps, chosen using the LEDs above the grid.
namespace acid303::seq {
class SequenceGate {
public:
    void reset() noexcept {
        noteCounts.fill(0);
        heldTotal = 0;
        stepPhase = 0.0;
        lastStep = -1;
    }
    void setTiming(int newStepsPerBeat, int newLength) noexcept {
        newStepsPerBeat = std::clamp(newStepsPerBeat, 1, 8);
        newLength = std::clamp(newLength, 1, 16);
        if (newLength != loopLength) {
            stepPhase = std::fmod(stepPhase, static_cast<double>(newLength));
            lastStep = -1; // Re-evaluate the step after changing loop length
        }
        stepsPerBeat = newStepsPerBeat;
        loopLength = newLength;
    }
    void noteOn(int midiNote) noexcept {
        if (midiNote < 0 || midiNote > 127) return;
        if (heldTotal == 0) {
            stepPhase = 0.0;
            lastStep = -1;
        }
        auto& count = noteCounts[static_cast<size_t>(midiNote)];
        if (count < 65535) { ++count; ++heldTotal; }
    }
    void noteOff(int midiNote) noexcept {
        if (midiNote < 0 || midiNote > 127) return;
        auto& count = noteCounts[static_cast<size_t>(midiNote)];
        if (count == 0) return;
        --count;
        --heldTotal;
        if (heldTotal == 0) { stepPhase = 0.0; lastStep = -1; }
    }
    bool active() const noexcept { return heldTotal > 0; }
    int stepToTrigger() noexcept {
        if (!active()) return -1;
        const int step = std::clamp(static_cast<int>(stepPhase), 0, loopLength - 1);
        if (step == lastStep) return -1;
        lastStep = step;
        return step;
    }
    void advance(float bpm, double sampleRate) noexcept {
        if (!active() || sampleRate <= 0.0) return;
        const double increment = static_cast<double>(stepsPerBeat)
            * std::clamp(static_cast<double>(bpm), 1.0, 999.0) / (60.0 * sampleRate);
        stepPhase += increment;
        if (stepPhase >= static_cast<double>(loopLength)) {
            stepPhase = std::fmod(stepPhase, static_cast<double>(loopLength));
            lastStep = -1; // especially needed for a one-step pattern
        }
    }
private:
    std::array<uint16_t, 128> noteCounts{};
    uint32_t heldTotal = 0;
    double stepPhase = 0.0;
    int lastStep = -1;
    int stepsPerBeat = 4;
    int loopLength = 16;
};
} // namespace acid303::seq
