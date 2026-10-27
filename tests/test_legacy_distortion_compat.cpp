// v2.0.6 revision B: Bitcrusher remains sample-identical to v2.0.5,
// Tube selection remains index 0 but intentionally uses the new sound.
#define acid303 acid303_legacy
#include "reference/Engine_v205.h"
#undef acid303
#include "../Source/Engine.h"
#include <iostream>
#include <cmath>
#include <algorithm>
int main() {
  double tubeDelta=0.0;
  for(int type:{0,1}) for(int sr:{44100,48000,96000})
   for(float drive:{0.12f,0.53f,0.96f}) {
    acid303_legacy::Engine old;acid303::Engine current;
    old.prepare(sr);current.prepare(sr);
    acid303_legacy::Controls a;acid303::Controls b;
    a.distOn=b.distOn=true;a.distType=b.distType=type;
    a.distDrive=b.distDrive=drive;
    for(int i=0;i<30000;++i) {
      if(i%11000==0) {old.noteOn(48,0.95f,a);current.noteOn(48,0.95f,b);}
      const auto x=old.render(a);const auto y=current.render(b);
      if(type==1 && (x.l!=y.l||x.r!=y.r)) {
        std::cerr<<"FAIL: Bitcrusher difference "<<sr<<"Hz "<<drive<<" at "<<i<<"\n";return 1;
      }
      if(type==0) tubeDelta+=std::abs(double(x.l)-double(y.l));
    }
   }
  if(tubeDelta<10.0) { std::cerr<<"FAIL: new Tube not distinct from old\n"; return 2; }
  std::cout<<"PASS Bitcrusher sample-identical across 9 cases; Tube intentionally changed (sum delta="<<tubeDelta<<")\n";
}
