#include "../Source/PatternBank.h"
#include "../Source/Sequencer.h"
#include "../Source/SequenceGate.h"
#include <cassert>
#include <iostream>
#include <array>
int main() {
    using namespace acid303;
    static_assert(bank::kTotalPatterns==64);
    std::array<bool,64> seen{};
    for (int group=0;group<8;++group) for(int p=0;p<8;++p) {
        const int index=bank::slot(group,p);
        assert(index>=0 && index<64);
        assert(!seen[index]); seen[index]=true;
        assert(bank::group(index)==group);
        assert(bank::pattern(index)==p);
        assert(bank::fromMidi(bank::toMidi(index))==index);
    }
    for(bool s:seen) assert(s);
    for(int n=0;n<128;++n) assert(bank::fromMidi(n)==(n>=36&&n<=99?n-36:-1));
    assert(bank::slot(8,0)==-1);
    assert(bank::toMidi(64)==-1);
    for(int pat=0;pat<64;++pat) for(int i=0;i<16;++i) {
        seq::Step step{i%3==0, static_cast<uint8_t>((pat+i)%128),i%5==0,i%7==0};
        const auto encoded=seq::pack(step); const auto decoded=seq::unpack(encoded);
        assert(decoded.on==step.on && decoded.note==step.note);
        assert(decoded.slide==step.slide && decoded.accent==step.accent);
    }
    seq::SequenceGate gate;
    gate.setTiming(4,16); gate.noteOn(0);
    assert(gate.stepToTrigger()==0);
    gate.noteOff(0); assert(!gate.active());
    std::cout << "PASS: 64 bank map, MIDI 36..99, 1024 packed steps, gate\n";
}
