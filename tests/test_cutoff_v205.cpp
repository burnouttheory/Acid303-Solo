#include "../Source/Engine.h"
#include <cmath>
#include <cassert>
#include <iostream>
#include <limits>
#include <algorithm>

using namespace acid303;
int main() {
    for (float sr: {44100.0f, 48000.0f, 96000.0f}) {
        Controls c;
        c.envMod=0.0f; c.mod=0.0f;
        float prev=0;
        for (int i=0;i<=100;i++) {
            c.cutoff=i/100.0f;
            const float now=cutoffFrequencyHz(c, 0, 0, 0, sr);
            assert(std::isfinite(now));
            assert(now > prev);
            assert(now <= sr*.42f);
            prev=now;
        }
        c.cutoff=0;const float dark=cutoffFrequencyHz(c,0,0,0,sr);
        c.cutoff=1;const float open=cutoffFrequencyHz(c,0,0,0,sr);
        assert(open / dark > 300.0f);
        std::cout << "frequency-range sr="<<sr<<" "<<dark<<" to "<<open<<" Hz\n";

        Engine e;
        e.prepare(sr);
        c.cutoff=.35f;
        c.envMod=.70f;
        c.reso=.85f;
        c.mod=0;
        e.noteOn(48, .9f, c);
        double sum=0;
        for (int i=0;i<120000;i++) {
            if (i == 20000) c.cutoff=0;
            if (i == 50000) c.cutoff=1;
            if (i == 80000) c.cutoff=.3f;
            const Stereo s = e.render(c);
            assert(std::isfinite(s.l) && std::isfinite(s.r));
            assert(std::abs(s.l)<=1.01 && std::abs(s.r)<=1.01);
            sum += std::abs(s.l);
        }
        assert(sum > 0.001);
    }
    Controls c; c.cutoff=0;c.envMod=.7f;c.accent=.68f;
    const float minVal=cutoffFrequencyHz(c, 1.15f, .8f, 0, 44100);
    c.cutoff=1;
    const float maxVal=cutoffFrequencyHz(c, 1.15f, .8f, 0, 44100);
    assert(maxVal/minVal > 100);
    std::cout<<"accented-envelope dark="<<minVal<<" open="<<maxVal<<"\n";
    std::cout<<"PASS: 3 sample rates, monotonic curve, wide range, long automation sweep\n";
}
