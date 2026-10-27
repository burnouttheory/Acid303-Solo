// Acid303 v2.0.6 native DSP tests -- no JUCE or Windows required.
#include "../Source/Engine.h"
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <numeric>
using namespace acid303;
static void require(bool ok, const char* what) { if (!ok) throw std::runtime_error(what); }
static std::vector<float> render(int sr, int mode, float drive, bool on=true, float wet=1.0f) {
    Engine e; e.prepare(sr); Controls c;
    c.distOn=on; c.distType=mode; c.distWet=wet; c.distDrive=drive;
    c.distEq=0.55f; c.distVolume=0.9f;
    c.cutoff=0.68f; c.reso=0.83f; c.envMod=0.7f; c.decay=0.8f;
    std::vector<float> v(static_cast<size_t>(sr));
    for(int i=0;i<sr;++i){
        if(i%(sr/4)==0) e.noteOn(45+i%2,0.9f,c);
        v[static_cast<size_t>(i)]=e.render(c).l;
    }
    return v;
}
static double diff(const std::vector<float>& x,const std::vector<float>& y){
    double sum=0; for(size_t i=0;i<x.size();i++)sum+=double(x[i]-y[i])*(x[i]-y[i]); return std::sqrt(sum/x.size());
}
int main(){
 try {
  for(int sr:{44100,48000,96000}) {
   const auto tube=render(sr,0,0.74f); const auto crusher=render(sr,1,0.74f);
   const auto acid=render(sr,0,0.74f); const auto acidSoft=render(sr,0,0.18f);
   const auto off=render(sr,0,0.74f,false);
   const auto dry=render(sr,0,0.74f,true,0.0f);
   require(diff(off,dry)<1e-8,"dry mix differs from bypass");
   require(diff(tube,acid)<1e-8,"Tube is not routed to new acid processing");
   require(diff(crusher,acid)>0.016,"Acid Drive not distinct from Bitcrusher");
   require(diff(acid,acidSoft)>0.016,"Drive modulation inaudible");
   for(float a:acid) require(std::isfinite(a)&&std::abs(a)<1.001f,"nonfinite or clipped output");
   Engine e; e.prepare(sr); Controls c; c.distOn=true; c.distType=0; c.distDrive=1.0f;
   for(int i=0;i<sr;++i){
     if(i%113==0)c.distType=(i/113)%2;
     if(i%113==1)c.distDrive=float((i/113)%10)/9;
     if(i%911==0)e.noteOn(36+(i/911)%24,0.9f,c);
     auto out=e.render(c);require(std::isfinite(out.l)&&std::isfinite(out.r),"mode switching is unstable");
   }
   std::cout << sr << "Hz: PASS | new-tube self delta="<<diff(tube,acid)
    << " acid/crusher="<<diff(crusher,acid)
    << " drive_delta="<<diff(acid,acidSoft)<<"\n";
  }
 } catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<"\n";return 1;}
 return 0;
}
