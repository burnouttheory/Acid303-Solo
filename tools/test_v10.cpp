#include "../Source/Engine.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>
using namespace acid303;

struct Measurements { double energy=0; double peak=0; double change=0; };
static std::vector<float> render(const Controls& c) {
    Engine e; e.prepare(44100); e.noteOn(42,0.95f,c);
    std::vector<float> data;
    data.reserve(8000);
    for (int i=0;i<8000;++i) {
        auto s=e.render(c);
        assert(std::isfinite(s.l) && std::isfinite(s.r));
        assert(std::abs(s.l) <= 1.001f && std::abs(s.r) <= 1.001f);
        data.push_back(s.l);
    }
    return data;
}
static double energy(const std::vector<float>& data) {
    double e=0;
    for (float x: data) e+=double(x)*x;
    return e;
}
static double rmsDifference(const std::vector<float>& a,const std::vector<float>& b) {
    double e=0;
    for (size_t i=0;i<a.size();++i) e+=double(a[i]-b[i])*double(a[i]-b[i]);
    return std::sqrt(e/a.size());
}
int main() {
    Controls c;
    c.masterVolume=1;
    const auto dry=render(c);
    c.distOn=true; c.distDrive=0.58f; c.distWet=0.72f;
    auto wet=render(c);
    const double diff=rmsDifference(dry,wet);
    assert(diff>0.09); // audible strong nonlinear saturation with factory settings
    c.distWet=0;
    auto bypassWet0=render(c);
    assert(rmsDifference(dry,bypassWet0)<1e-8); // exactly transparent when dry
    c.distOn=false; c.distDrive=1; c.distWet=1;
    assert(rmsDifference(dry,render(c))<1e-8); // truly bypassed
    c.masterVolume=0;
    assert(energy(render(c)) < 1e-10); // new master slider mutes all output
    c.masterVolume=0.5f;
    const auto half=render(c);
    assert(energy(half)>0);
    assert(std::abs(energy(half)/energy(dry)-0.25)<1.0e-5); // post-FX linear gain
    c.masterVolume=1;
    Engine e; e.prepare(44100);
    c.bpm=120;c.delayTime=0.375f;
    auto d120=e.delaySamples(c);
    c.bpm=240;
    auto d240=e.delaySamples(c);
    assert(std::abs(d120-2*d240)<=2); // host BPM controls delay
    for (int k=0;k<=10;++k) {
        c.cutoff = k/10.0f; c.reso=1.0f; c.envMod=1;
        c.distOn=true;c.distWet=1; c.distDrive=1; c.masterVolume=1;
        auto out=render(c); assert(energy(out)>0);
    }
    std::cout << "OK v0.10 : acid distortion rms delta " << diff
              << "; clean bypass; master mute and scaling; tempo sync; stable resonance\n";
}
