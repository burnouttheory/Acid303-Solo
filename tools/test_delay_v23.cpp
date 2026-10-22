#include "../Source/Engine.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>
using namespace acid303;

struct Response {
    std::vector<float> l,r;
};
static Response impulseDelay(int mode, float width, float wet=1.0f) {
    Controls c;
    c.distOn = false; c.compOn=false; c.reverbOn=false;
    c.delayOn=true; c.delayMode=mode; c.delayWidth=width;
    c.delayWet=wet; c.delayFeedback=0.72f; c.delayTime=0.0f;
    c.bpm=240; c.masterVolume=1.0f;
    Engine engine;engine.prepare(44100);
    Response out;
    out.l.reserve(20000);out.r.reserve(20000);
    engine.noteOn(48, 0.95f, c);
    for(int i=0;i<20000;++i) {
        if (i==20) engine.allNotesOff();
        auto x=engine.render(c);
        assert(std::isfinite(x.l) && std::isfinite(x.r));
        out.l.push_back(x.l);out.r.push_back(x.r);
    }
    return out;
}
static double energy(const std::vector<float>& x, int a,int b) {
    double e=0;for(int i=a;i<b;++i) e+=double(x[static_cast<size_t>(i)])*x[static_cast<size_t>(i)];return e;
}
int main() {
    Controls c;
    Engine e; e.prepare(44100);
    c.bpm=120;c.delayTime=0.375f;
    auto n120=e.delaySamples(c);c.bpm=240;auto n240=e.delaySamples(c);
    assert(std::abs(n120-2*n240)<=2); // remains host-tempo synced
    for(int mode=0;mode<=1;++mode) {
        auto mono=impulseDelay(mode,0);
        auto wide=impulseDelay(mode,1);
        for (size_t i=0;i<mono.l.size();++i) assert(std::abs(mono.l[i]-mono.r[i])<1e-6f);
        // Width cannot remain a no-op.
        double difference=0;for (size_t i=0;i<wide.l.size();++i) difference+=std::abs(wide.l[i]-wide.r[i]);
        assert(difference > 0.01);
        auto dry=impulseDelay(mode,1,0);
        // No echoes before any timed repeats when 100% wet.
        assert(energy(wide.l,0,2500)<1e-10);
        assert(energy(dry.l,0,2500)>0.001);
    }
    // At 240 BPM, time setting 0 is a sixteenth note = 2756 samples.
    // Ping-pong alternates left (first repeat) then right (second repeat).
    auto ping=impulseDelay(1,1);
    auto firstL=energy(ping.l,2756,3156), firstR=energy(ping.r,2756,3156);
    auto secondL=energy(ping.l,5512,5912), secondR=energy(ping.r,5512,5912);
    std::cout << "pingpong first L/R "<<firstL<<" / "<<firstR<<"; second L/R "<<secondL<<" / "<<secondR<<""<<std::endl;
    assert(firstL > firstR*200.0);
    assert(secondR > secondL*2.0);
    // Changing the routing clears any leftover echo from previous mode.
    c.delayOn=true;c.delayWet=1;c.delayTime=0;c.delayFeedback=0.72f;c.bpm=240;
    c.delayMode=0;
    Engine runtime;runtime.prepare(44100);runtime.noteOn(48,0.95f,c);
    for(int i=0;i<6000;++i)runtime.render(c);
    runtime.allNotesOff();c.delayMode=1;
    auto switchSample=runtime.render(c);
    assert(std::abs(switchSample.l) < 1e-5f && std::abs(switchSample.r) < 1e-5f);
    c.delayOn=false; runtime.render(c);
    c.delayOn=true;auto restored=runtime.render(c);
    assert(std::abs(restored.l)<1e-5f && std::abs(restored.r)<1e-5f);
    std::cout << "OK: Normal/Ping-Pong, WIDTH, bypass, switched routing, FL Studio BPM sync"<<std::endl;
}
