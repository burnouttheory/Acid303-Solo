#include "Engine.h"
#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>

using namespace acid303;
static double run(const Controls& c, int note=48, float vel=0.65f, bool slide=false) {
    Engine e; e.prepare(44100);
    e.noteOn(note,vel,c);
    if (slide) {for (int i=0;i<2000;++i) e.render(c);e.noteOn(note+5,vel,c);}
    double energy=0;
    for (int i=0;i<6000;++i) {
        const auto s=e.render(c);
        assert(std::isfinite(s.l) && std::isfinite(s.r));
        assert(std::abs(s.l)<=1.01f && std::abs(s.r)<=1.01f);
        energy+=double(s.l)*s.l+double(s.r)*s.r;
    }
    return energy;
}
int main() {
    Controls c;
    Engine engine; engine.prepare(44100);
    // One beat is 22050 samples at 120 bpm, 11025 at 240 bpm.
    c.bpm=120;c.delayTime=0.375f;
    const int n120=engine.delaySamples(c);
    c.bpm=240;
    const int n240=engine.delaySamples(c);
    assert(n120>0 && n240>0 && std::abs(n120-2*n240)<=2);
    const double base=run(c);
    assert(base>0.00001);
    c.volume=0; assert(run(c)<base*0.0001);
    c.volume=0.7f;
    c.wave=1; assert(std::abs(run(c)-base)>0.0001);
    c.wave=0;c.shape=0.1f;assert(std::abs(run(c)-base)>0.0001);
    c.shape=0.5f;c.cutoff=0.9f;assert(std::abs(run(c)-base)>0.0001);
    c.cutoff=0.44f;c.reso=0.1f;assert(std::abs(run(c)-base)>0.0001);
    c.reso=0.6f;c.mod=0.8f;c.lfoRate=11.0f;assert(std::abs(run(c)-base)>0.0001);
    c.mod=0;c.envMod=1;assert(std::abs(run(c)-base)>0.0001);
    c.envMod=0.58f;c.accent=1;assert(std::abs(run(c,48,0.99f)-run(c,48,0.2f))>0.0001);
    c.accent=0.68f;
    // OFF genuinely bypasses even extreme parameter settings.
    const double clean=run(c);
    c.distDrive=1;c.distVolume=0;c.distEq=0;
    assert(std::abs(run(c)-clean)<1.0e-9); // distortion OFF
    c.distOn=true;c.distWet=0;
    assert(std::abs(run(c)-clean)<1.0e-9); // full dry, even with LEVEL=0
    c.distDrive=0.9f;c.distEq=0.96f;c.distVolume=1;
    c.distWet=1;
    assert(std::abs(run(c)-clean)>0.0001); // wet sound is different
    c.distOn=false;
    assert(std::abs(run(c)-clean)<1.0e-9); // bypass transparent again
    c.distDrive=0;c.distVolume=0.85f;c.distEq=0.5f;

    c.delayTime=0;c.delayFeedback=0.35f;
    c.delayOn=true;c.delayWet=0;
    assert(std::abs(run(c)-clean)<1.0e-9); // delay 100% dry
    c.delayWet=0.7f;
    assert(std::abs(run(c)-clean)>0.0001);
    c.delayOn=false;
    assert(std::abs(run(c)-clean)<1.0e-9);
    c.reverbOn=true;c.reverbSize=0.8f;c.reverbVolume=0.85f;c.reverbWet=0;
    assert(std::abs(run(c)-clean)<1.0e-9);
    c.reverbWet=0.7f;
    assert(std::abs(run(c)-clean)>0.0001);
    c.reverbOn=false;
    assert(std::abs(run(c)-clean)<1.0e-9);

    // Runtime switches: bypass clears the delay tail immediately, including fast reactivation.
    c.delayOn=true;c.delayWet=1;c.delayTime=0;c.bpm=240;
    Engine runtime;runtime.prepare(44100);
    runtime.noteOn(48,0.7f,c);
    for (int j=0;j<7000;++j) runtime.render(c);
    c.delayOn=false;
    auto off=runtime.render(c);
    assert(std::isfinite(off.l));
    c.delayOn=true;
    c.delayWet=1;
    auto backOn=runtime.render(c);
    assert(std::abs(backOn.l) < 0.0001f); // old echoes may not reappear
    c.delayOn=false;

    c.reverbOn=true;c.reverbSize=0.9f;c.reverbWet=1;
    for (int j=0;j<5000;++j) runtime.render(c);
    c.reverbOn=false;
    runtime.render(c);
    c.reverbOn=true;
    assert(std::isfinite(runtime.render(c).l));
    c.reverbOn=false;
    assert(std::isfinite(run(c,48,0.8f,true)));
    engine.noteOn(43,0.6f,c);engine.noteOn(48,0.8f,c);engine.noteOff(48,c);engine.noteOff(43,c);
    for (int i=0;i<1000;++i) assert(std::isfinite(engine.render(c).l));
    std::cout<<"OK: MIDI bass, bypass, independent dry/wet, clean FX resets, FL Studio tempo sync, legato\n";
}
