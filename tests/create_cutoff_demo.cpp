#include "../Source/Engine.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <cstdint>
#include <cmath>
#include <algorithm>

static void le16(std::ofstream& o, uint16_t n) { o.put(char(n)); o.put(char(n>>8)); }
static void le32(std::ofstream& o, uint32_t n) { le16(o,n);le16(o,uint16_t(n>>16)); }
int main(int argc,char** argv) {
    if(argc!=2) return 1;
    const int sr=44100;
    std::vector<int16_t> samples;
    acid303::Controls c;
    c.wave=0.0f;c.reso=.68f;c.envMod=.70f;c.accent=.68f;
    c.volume=.78f;c.masterVolume=.90f;c.decay=.67f;
    for(float cutoff: {0.f,.25f,.5f,.75f,1.f}) {
        acid303::Engine e;
        e.prepare(sr);
        c.cutoff=cutoff;
        e.noteOn(43,1.0f,c);
        for(int n=0;n<sr*.40;n++) {
            const auto out=e.render(c);
            samples.push_back(int16_t(std::clamp(out.l,-1.f,1.f)*32760));
        }
        for(int n=0;n<sr/12;n++) samples.push_back(0);
    }
    std::ofstream o(argv[1],std::ios::binary);
    o.write("RIFF",4); le32(o,uint32_t(36+samples.size()*2));o.write("WAVEfmt ",8);
    le32(o,16);le16(o,1);le16(o,1);le32(o,sr);le32(o,sr*2);le16(o,2);le16(o,16);
    o.write("data",4);le32(o,uint32_t(samples.size()*2));
    for(auto s:samples) le16(o,uint16_t(s));
    std::cout<<"Generated 0%, 25%, 50%, 75%, 100% CUTOFF (saw G2)\n";
}
