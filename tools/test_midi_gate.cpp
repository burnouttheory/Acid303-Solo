#include "../Source/SequenceGate.h"
#include "../Source/Sequencer.h"
#include "../Source/Engine.h"
#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>

using acid303::seq::SequenceGate;

static std::vector<int> simulate(SequenceGate& g, int samples, float bpm, int sampleRate) {
    std::vector<int> trigs;
    for (int n=0;n<samples;++n) {
        if (const int s=g.stepToTrigger(); s>=0) trigs.push_back(s);
        g.advance(bpm, sampleRate);
    }
    return trigs;
}
int main() {
    SequenceGate gate;
    assert(!gate.active());
    assert(simulate(gate, 48000, 120, 48000).empty()); // PLAY with no held MIDI note = silence

    // One C5 note over exactly 4 bars in 4/4 @ 120 BPM: four 16-step cycles.
    gate.noteOn(72); // C5 is a gate, NOT the sequenced pitch
    const auto fourBars = simulate(gate, 48000 * 8, 120, 48000);
    assert(fourBars.size() == 64);
    for (size_t i=0;i<fourBars.size();++i)
        assert(fourBars[i]==static_cast<int>(i%16));
    gate.noteOff(72);
    assert(!gate.active());
    assert(simulate(gate, 48000, 120, 48000).empty());

    // Re-trigger anywhere on timeline: ALWAYS starts at step 1.
    gate.noteOn(60);
    const auto first = simulate(gate, 800, 143, 48000);
    assert(first.size()==1 && first[0]==0);
    gate.noteOff(60);
    assert(!gate.active());

    // Overlapping notes keep the gate open, even if the first note ends.
    gate.noteOn(72);
    gate.noteOn(74);
    simulate(gate, 12000, 120, 48000);
    gate.noteOff(72);
    assert(gate.active());
    const auto afterFirstOff=simulate(gate, 12000, 120, 48000);
    assert(!afterFirstOff.empty());
    gate.noteOff(74);
    assert(!gate.active());
    assert(simulate(gate, 48000, 120, 48000).empty());

    // Overlapping note-on of identical pitch is reference counted.
    gate.noteOn(72);
    gate.noteOn(72);
    gate.noteOff(72);
    assert(gate.active());
    gate.noteOff(72);
    assert(!gate.active());

    // Tempo change still advances by beat; 1 bar at 120, 1 at 180.
    gate.noteOn(72);
    const auto at120=simulate(gate, 96000, 120, 48000);
    const auto at180=simulate(gate, 64000, 180, 48000);
    assert(at120.size()==16);
    assert(at180.size()==16);
    assert(at180[0]==0 && at180.back()==15);
    gate.reset(); // FL stop/seek
    assert(!gate.active());

    acid303::Engine engine;
    engine.prepare(48000.0);
    acid303::Controls c;
    float total=0;
    engine.noteOn(48,0.9f,c);
    for (int n=0;n<4800;++n) {
        const auto out=engine.render(c);
        assert(std::isfinite(out.l) && std::isfinite(out.r));
        total += std::fabs(out.l)+std::fabs(out.r);
    }
    assert(total>0.01f);
    std::cout << "PASS: 4 bars=64 steps, MIDI note off, overlap, retrigger, tempo changes, sound path\n";
}
