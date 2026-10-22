#include "Engine.h"
#include <cmath>
#include <iostream>
#include <cassert>
using acid303::Controls;
using acid303::Engine;

static float rms(Controls c, int bandChoice) {
    Engine e; e.prepare(44100);
    e.noteOn(48,1.0f,c);
    double sum=0.0;
    for(int i=0;i<44100;++i) {
        const auto s=e.render(c);
        assert(std::isfinite(s.l) && std::isfinite(s.r));
        if(i>3000) sum+=double(s.l)*s.l;
    }
    (void)bandChoice;
    return float(std::sqrt(sum/41000.0));
}
int main(){
    Controls c;
    c.compOn=false;
    const float dry = rms(c,0);
    c.compOn=true;
    c.compWet=0.0f;
    const float zeroWet=rms(c,0);
    assert(std::abs(zeroWet-dry)<1e-6f); // true, level-independent bypass
    c.compWet=1.0f;
    c.compLow=c.compMid=c.compHigh=1.0f;
    c.compGain=1.0f;
    const float compressed=rms(c,0);
    assert(std::abs(compressed-dry)>1e-4f);
    c.compGain=0.0f;
    assert(rms(c,0)<1e-7f);
    c.compGain=1.0f;
    c.compLow=1.0f;c.compMid=c.compHigh=0.0f;
    const float low=rms(c,0);
    c.compLow=0.0f;c.compMid=1.0f;c.compHigh=0.0f;
    const float mid=rms(c,0);
    c.compLow=0.0f;c.compMid=0.0f;c.compHigh=1.0f;
    const float high=rms(c,0);
    assert(std::abs(low-mid)>1e-5f || std::abs(mid-high)>1e-5f);
    std::cout << "PASS: compressor bypass, dry/wet, gain, finite sample and 3 band intensities.\n";
    std::cout << "RMS dry/compressed/low/mid/high: " <<dry<<"/"<<compressed<<"/"<<low<<"/"<<mid<<"/"<<high<<"\n";
}
