#include "../Source/Engine.h"
#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>
#include <array>
using namespace acid303;
static std::vector<float> generate(const Controls& c) {
    Engine e; e.prepare(44100); e.noteOn(48, 0.95f, c);
    std::vector<float> out;
    for (int n = 0; n < 18000; ++n) {
        auto stereo=e.render(c);
        assert(std::isfinite(stereo.l) && std::isfinite(stereo.r));
        assert(std::abs(stereo.l) <= 1.001f && std::abs(stereo.r) <= 1.001f);
        out.push_back(stereo.l);
    }
    return out;
}
static double difference(const std::vector<float>& a, const std::vector<float>& b) {
    double sum = 0;
    for (size_t k=0; k<a.size(); ++k) sum += double(a[k]-b[k])*double(a[k]-b[k]);
    return std::sqrt(sum/a.size());
}
int main() {
    Controls c;
    c.masterVolume=1.0f;
    const auto dry=generate(c);
    std::array<std::vector<float>,2> modes;
    for (int mode = 0; mode < 2; ++mode) {
        c.distOn=true;
        c.distType=mode;
        c.distDrive=0.62f;
        c.distWet=1.0f;
        c.distVolume=0.85f;
        modes[mode]=generate(c);
        assert(difference(dry,modes[mode]) > 0.02);
        for (int other=0; other<mode; ++other)
            assert(difference(modes[mode],modes[other]) > 0.004);
        c.distWet=0.0f;
        assert(difference(dry, generate(c)) < 1e-7);
        c.distWet=1.0f;
        c.distOn=false;
        assert(difference(dry, generate(c)) < 1e-7);
    }
    c.distOn=true;c.distType=1;
    Engine changing;changing.prepare(44100);changing.noteOn(48,0.9f,c);
    for (int i=0;i<18000;++i) {
        if (i % 3000 == 0) c.distType=(i / 3000)%2;
        auto stereo=changing.render(c);
        assert(std::isfinite(stereo.l) && std::isfinite(stereo.r));
        assert(std::abs(stereo.l)<=1.001 && std::abs(stereo.r)<=1.001);
    }
    // Delay remains clocked from the host-provided tempo.
    c.delayOn=true;c.bpm=95;
    const auto first=changing.delaySamples(c);
    c.bpm=190;
    assert(std::abs(first - 2*changing.delaySamples(c)) <= 2);
    std::cout << "OK v0.12: Tube and Bitcrusher are distinct, true bypass/dry, mid-note mode changes, host tempo sync\n";
}
