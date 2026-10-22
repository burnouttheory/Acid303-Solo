#include "../Source/SequenceGate.h"
#include <cassert>
#include <vector>
#include <iostream>
#include <cmath>
using acid303::seq::SequenceGate;
static std::vector<int> run(SequenceGate& gate,int samples, float bpm=120.0f,int rate=48000) {
    std::vector<int> out;
    for (int i=0;i<samples;++i) {
        int step=gate.stepToTrigger();
        if (step>=0) out.push_back(step);
        gate.advance(bpm,rate);
    }
    return out;
}
int main() {
    const int sr=48000;
    const int bar = 96000; // 4 quarters @120 BPM
    for (int i=0; i<3; ++i) {
        SequenceGate g;
        int perBeat = 1 << i;
        g.setTiming(perBeat,16);
        assert(run(g,bar).empty()); // host transport but no MIDI note = no sequence
        g.noteOn(72);
        auto notes = run(g,bar);
        assert(static_cast<int>(notes.size()) == 4*perBeat);
        for (int j=0;j<static_cast<int>(notes.size());++j) assert(notes[j]==j);
        g.noteOff(72);
        assert(run(g,bar).empty());
    }
    for (int length=1;length<=16;++length) {
        SequenceGate g;
        g.setTiming(4,length);
        g.noteOn(72);
        auto notes = run(g,bar * 4);
        assert(notes.size()==64);
        for (size_t j=0;j<notes.size();++j) assert(notes[j]==static_cast<int>(j % length));
        g.noteOff(72);
        assert(run(g,bar).empty());
    }
    SequenceGate g;
    g.setTiming(2,12);
    g.noteOn(72);
    auto a = run(g,bar*3);
    assert(a.size()==24);
    for (int i=0;i<24;++i) assert(a[i]==i%12);
    g.setTiming(4,8);
    auto b=run(g,bar);
    assert(!b.empty());
    for (int s: b) assert(s>=0 && s<8);
    g.noteOff(72);
    g.noteOn(72);
    auto c=run(g,1000);
    assert(c.size()==1 && c[0]==0);
    g.reset();
    assert(!g.active());
    std::cout << "PASS: 1/4, 1/8, 1/16, lengths 1..16, sustain gate, variable tempo; step 1 restart\n";
}
