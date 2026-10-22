#include "Engine.h"
#include <cassert>
#include <cmath>
#include <iostream>

// At identical instrument/room settings, raising wet reverb level must
// increase the post-note tail and should remain finite across settings.
static double tailEnergy(float level, float wet, float room, bool enabled)
{
    acid303::Engine e;
    e.prepare(44100.0);
    acid303::Controls c;
    c.reverbOn=enabled;
    c.reverbSize=room;
    c.reverbVolume=level;
    c.reverbWet=wet;
    c.distOn=false;
    c.delayOn=false;
    e.noteOn(48, 0.7f, c);
    double energy=0.0;
    for (int i=0; i<66150; ++i) {
        if (i==8820) e.noteOff(48, c);
        auto out=e.render(c);
        assert(std::isfinite(out.l) && std::isfinite(out.r));
        assert(std::abs(out.l) <= 1.01f && std::abs(out.r) <= 1.01f);
        if (i>=22050) energy+=out.l*out.l+out.r*out.r;
    }
    return energy;
}
int main()
{
    const double little=tailEnergy(0.15f, 1.0f, 0.70f, true);
    const double loud=tailEnergy(0.60f, 1.0f, 0.70f, true);
    const double dry=tailEnergy(0.99f, 0.0f, 0.70f, true);
    const double bypass=tailEnergy(0.99f, 1.0f, 0.70f, false);
    assert(loud>little*2.5);
    assert(std::abs(dry-bypass) < 1.0e-7);
    assert(tailEnergy(1.0f, 1.0f, 1.0f, true)>0.001);
    std::cout << "OK: stronger reverb volume, 100% dry transparent, bypass transparent, full-room finite\n";
}
