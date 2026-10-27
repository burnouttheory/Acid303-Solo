#include "../Source/Engine.h"
#include <cstdint>
#include <fstream>
#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>
static void u16(std::ofstream&f,uint16_t n){f.put(char(n&255));f.put(char(n>>8));}
static void u32(std::ofstream&f,uint32_t n){u16(f,uint16_t(n));u16(f,uint16_t(n>>16));}
int main(int argc,char**argv){
 if(argc!=2)return 2;
 const int sampleRate=48000, duration=14, frames=sampleRate*duration;
 std::vector<int16_t> audio;audio.reserve(frames);
 acid303::Engine engine;engine.prepare(sampleRate);acid303::Controls c;
 c.cutoff=0.70f;c.reso=0.84f;c.decay=0.62f;c.envMod=0.68f;c.accent=0.85f;
 c.distOn=true;c.distWet=0.9f;c.distVolume=0.85f;c.distEq=0.57f;
 const int seq[]{36,39,43,46,36,48,43,41};
 for(int i=0;i<frames;++i){
  const int section=i/(2*sampleRate);
  switch(section){
    case 0: c.distType=0;c.distDrive=0.30f;break; // New Tube low drive
    case 1: c.distType=0;c.distDrive=0.70f;break; // New Tube strong
    case 2: c.distType=0;c.distDrive=0.90f;break; // New Tube intense
    case 3: c.distType=1;c.distDrive=0.65f;break; // Bitcrusher
    case 4: c.distType=1;c.distDrive=0.90f;break; // Bitcrusher strong
    case 5: c.distType=0;c.distDrive=0.84f;c.distEq=0.30f;break;
    default:c.distType=0;c.distDrive=0.84f;c.distEq=0.85f;break;
  }
  if(i%(sampleRate/4)==0){int idx=(i/(sampleRate/4))%8;engine.noteOn(seq[idx],idx%4==0?1.0f:0.65f,c);}
  const float out=engine.render(c).l;
  const float start=i% (2*sampleRate);
  const float edge=std::min(start,float(2*sampleRate)-start);
  const float fade=std::min(1.0f,edge/350.0f);
  audio.push_back(int16_t(std::lround(std::clamp(out*fade*0.85f,-1.0f,1.0f)*32767.0f)));
 }
 std::ofstream f(argv[1],std::ios::binary);
 f.write("RIFF",4);u32(f,36+uint32_t(audio.size()*2));f.write("WAVEfmt ",8);
 u32(f,16);u16(f,1);u16(f,1);u32(f,sampleRate);u32(f,sampleRate*2);u16(f,2);u16(f,16);
 f.write("data",4);u32(f,uint32_t(audio.size()*2));
 for(int16_t n:audio)u16(f,uint16_t(n));
 std::cout<<"WAV generated: "<<argv[1]<<"\n";
}
