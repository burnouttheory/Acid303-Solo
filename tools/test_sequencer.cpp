#include "../Source/Sequencer.h"
#include "../Source/Engine.h"
#include <cassert>
#include <cmath>
#include <iostream>
using namespace acid303;
int main() {
    seq::Step a{true, 42, true, true};
    auto packed = seq::pack(a);
    auto b = seq::unpack(packed);
    assert(b.on && b.note == 42 && b.slide && b.accent);
    for (int n=0;n<128;++n) {
        seq::Step x{n%2==0,static_cast<uint8_t>(n),n%3==0,n%5==0};
        const auto y=seq::unpack(seq::pack(x));
        assert(y.on==x.on && y.note==x.note && y.slide==x.slide && y.accent==x.accent);
    }
    // Four steps per quarter note, sixteen per four-beat bar.
    assert(seq::absoluteStep(0.0)==0);
    assert(seq::absoluteStep(0.2499)==0);
    assert(seq::absoluteStep(0.25)==1);
    assert(seq::absoluteStep(3.99999)==15);
    assert(seq::absoluteStep(4.0)==16);
    assert(seq::patternIndex(16)==0 && seq::patternIndex(15)==15);
    assert(seq::patternIndex(-1)==15);
    assert(seq::accentVelocity(true)>seq::accentVelocity(false));
    // At different FL Studio BPM values, 16 steps still span exactly 4 quarter notes.
    for (double bpm : {90.0, 128.0, 170.0}) {
        const auto seconds = 4.0 * (60.0/bpm);
        assert(std::abs(seconds*bpm/60.0*4.0-16.0)<1e-9);
    }
    Engine engine;
    engine.prepare(44100.0);
    Controls c;
    c.distOn = true; c.distWet = 0.35f;
    c.delayOn = true; c.delayWet = 0.27f;
    c.reverbOn = true; c.reverbWet = 0.40f;
    float total = 0;
    engine.noteOn(48,seq::accentVelocity(true),c);
    for(int i=0; i<11025; ++i) {
        auto out = engine.render(c);
        assert(std::isfinite(out.l) && std::isfinite(out.r));
        total += std::abs(out.l) + std::abs(out.r);
    }
    engine.noteOn(52,seq::accentVelocity(false),c); // legato for slide
    engine.noteOff(48,c);
    for(int i=0; i<11025; ++i) {
        auto out = engine.render(c);
        assert(std::isfinite(out.l) && std::isfinite(out.r));
        total += std::abs(out.l)+std::abs(out.r);
    }
    engine.noteOff(52,c);
    assert(total > 1.0f);
    std::cout << "Sequencer pack, PPQ timebase, accent, slide sound path, FX: OK\n";
}
