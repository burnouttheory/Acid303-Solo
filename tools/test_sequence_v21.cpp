#include "../Source/SequenceGate.h"
#include <cassert>
#include <iostream>
#include <vector>
using acid303::seq::SequenceGate;
static std::vector<int> run(SequenceGate& gate,int samples, float bpm=120.0f, int rate=48000) {
    std::vector<int> out;
    for (int i=0;i<samples;++i) {
        int step=gate.stepToTrigger();
        if (step>=0) out.push_back(step);
        gate.advance(bpm,rate);
    }
    return out;
}
int main() {
    const int bar=96000;
    for (int i=0;i<4;++i) {
        const int perBeat=1<<i;
        SequenceGate gate;
        gate.setTiming(perBeat,16);
        assert(run(gate,bar).empty());
        gate.noteOn(72);
        const auto triggered=run(gate,bar);
        assert(static_cast<int>(triggered.size())==4*perBeat);
        for (int j=0;j<static_cast<int>(triggered.size());++j) assert(triggered[j]==j%16);
        gate.noteOff(72);
        assert(run(gate,bar).empty());
    }
    for (int length=1;length<=16;++length) {
        SequenceGate gate;
        gate.setTiming(8,length);
        gate.noteOn(72);
        auto triggered=run(gate,bar*2);
        assert(triggered.size()==64);
        for (size_t j=0;j<triggered.size();++j) assert(triggered[j]==static_cast<int>(j%length));
        gate.noteOff(72);
        assert(run(gate,bar).empty());
        gate.noteOn(72);
        auto restarted=run(gate,500);
        assert(restarted.size()==1 && restarted.front()==0);
    }
    SequenceGate gate;
    gate.setTiming(8,16);
    gate.noteOn(72);
    assert(run(gate,12000,120).size()==4);
    assert(run(gate,24000,60).size()==4);
    std::cout << "PASS: 1/4, 1/8, 1/16, 1/32, 1..16 length, MIDI gate, restart, BPM changes\n";
}
