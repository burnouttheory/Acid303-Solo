#pragma once
// Self-contained Standard MIDI File (SMF) type 0 encoder/decoder for Acid303.
// JUCE-independent to allow deterministic off-line tests.
#include "Sequencer.h"
#include <array>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <string>
#include <vector>

namespace acid303::midi {
constexpr int ppq = 960;
struct Pattern {
    std::array<seq::Step, 16> steps{};
    int length = 16;
    int rate = 2; // 0=quarter, 1=eighth, 2=sixteenth, 3=thirty-second
    bool nativeMetadata = false;
};
struct Result {
    bool ok = false;
    std::string error;
    Pattern pattern{};
};
inline int stepTicks(int rate) { return ppq >> std::clamp(rate, 0, 3); }

namespace detail {
inline void be16(std::vector<uint8_t>& b, uint32_t v) {
    b.push_back(uint8_t((v >> 8) & 0xff)); b.push_back(uint8_t(v & 0xff));
}
inline void be32(std::vector<uint8_t>& b, uint32_t v) {
    for (int s : {24, 16, 8, 0}) b.push_back(uint8_t((v >> s) & 0xff));
}
inline uint32_t readBE(const std::vector<uint8_t>& b, size_t off, int len) {
    uint32_t v = 0; for (int i=0;i<len;++i) v = (v << 8) | b[off+size_t(i)]; return v;
}
inline void vlq(std::vector<uint8_t>& b, uint32_t value) {
    uint8_t tmp[5]; int count = 0;
    tmp[count++] = uint8_t(value & 0x7f);
    while ((value >>= 7) != 0) tmp[count++] = uint8_t((value & 0x7f) | 0x80);
    for (int i=count-1;i>=0;--i) b.push_back(tmp[i]);
}
inline bool takeVlq(const std::vector<uint8_t>& b, size_t& pos, size_t end, uint32_t& value) {
    value = 0;
    for (int i=0;i<4;++i) {
        if (pos >= end) return false;
        uint8_t v = b[pos++];
        value = (value << 7) | (v & 0x7f);
        if ((v & 0x80) == 0) return true;
    }
    return false;
}
inline std::string hex32(uint32_t v) {
    const char* h = "0123456789ABCDEF";
    std::string s(8,'0');
    for (int i=7;i>=0;--i) { s[size_t(i)] = h[v & 15]; v >>= 4; }
    return s;
}
inline bool parseHex8(const std::string& s, size_t off, uint32_t& v) {
    if (off + 8 > s.size()) return false;
    v = 0;
    for (size_t j=0;j<8;++j) {
        const char c=s[off+j];
        int x = c>='0' && c<='9' ? c-'0' : c>='A' && c<='F' ? c-'A'+10 : c>='a' && c<='f' ? c-'a'+10 : -1;
        if (x<0) return false;
        v = (v<<4) | uint32_t(x);
    }
    return true;
}
inline std::string metadata(const Pattern& p) {
    std::string text="ACID303_PATTERN_V1|RATE=" + std::to_string(std::clamp(p.rate,0,3))
        + "|LENGTH=" + std::to_string(std::clamp(p.length,1,16)) + "|STEPS=";
    for (auto s:p.steps) text+=hex32(seq::pack(s));
    return text;
}
inline bool readMetadata(const std::string& text, Pattern& out) {
    const std::string prefix="ACID303_PATTERN_V1|RATE=";
    if (text.compare(0, prefix.size(), prefix)!=0) return false;
    const auto a=text.find("|LENGTH=",prefix.size());
    const auto b=text.find("|STEPS=",a==std::string::npos ? 0 : a+8);
    if (a==std::string::npos || b==std::string::npos) return false;
    try {
        int rate=std::stoi(text.substr(prefix.size(),a-prefix.size()));
        int length=std::stoi(text.substr(a+8,b-(a+8)));
        if (rate<0 || rate>3 || length<1 || length>16 || text.size()!=b+7+128) return false;
        Pattern result;
        for(int i=0;i<16;++i) {
            uint32_t packed=0;
            if(!parseHex8(text,b+7+size_t(i)*8,packed)) return false;
            result.steps[size_t(i)]=seq::unpack(packed);
        }
        result.rate=rate; result.length=length; result.nativeMetadata=true;
        out=result;
        return true;
    } catch (...) { return false; }
}
struct Event {
    uint32_t tick{};
    int priority{};
    std::vector<uint8_t> bytes;
};
struct Note {
    uint32_t start{};
    uint32_t end{};
    uint8_t pitch{};
    uint8_t velocity{};
    int track{};
};
} // namespace detail

inline std::vector<uint8_t> encode(const Pattern& input) {
    Pattern p = input;
    p.length=std::clamp(p.length,1,16);
    p.rate=std::clamp(p.rate,0,3);
    const uint32_t step=uint32_t(stepTicks(p.rate));
    std::vector<detail::Event> events;
    const std::string meta=detail::metadata(p);
    std::vector<uint8_t> metaBytes{0xff,0x01}; // Standard text meta-event, safely ignored by other DAWs
    detail::vlq(metaBytes,uint32_t(meta.size()));
    metaBytes.insert(metaBytes.end(),meta.begin(),meta.end());
    events.push_back({0,0,std::move(metaBytes)});
    uint32_t finalTick=uint32_t(p.length)*step;
    for (int i=0;i<p.length;++i) {
        const auto s=p.steps[size_t(i)];
        if (!s.on) continue;
        const uint32_t begin=uint32_t(i)*step;
        // When the NEXT note is marked SLIDE, extend this note a little beyond
        // the next Note-On. DAWs interpret the overlap as legato. Keep native
        // metadata as the lossless representation, including slide on silent steps.
        uint32_t off=begin + step - 1;
        if (i+1 < p.length && p.steps[size_t(i+1)].on && p.steps[size_t(i+1)].slide)
            off=begin+step+std::max(uint32_t(1),step/5);
        finalTick=std::max(finalTick,off);
        const uint8_t pitch=uint8_t(std::clamp<int>(s.note,0,127));
        events.push_back({begin,2,{0x90,pitch,uint8_t(s.accent?120:74)}});
        events.push_back({off,1,{0x80,pitch,0x00}});
    }
    events.push_back({finalTick+1,3,{0xff,0x2f,0x00}});
    std::stable_sort(events.begin(),events.end(),[](const auto& a,const auto& b){
        return a.tick==b.tick ? a.priority<b.priority : a.tick<b.tick;
    });
    std::vector<uint8_t> track;
    uint32_t last=0;
    for(const auto& e:events) {
        detail::vlq(track,e.tick-last);
        track.insert(track.end(),e.bytes.begin(),e.bytes.end());
        last=e.tick;
    }
    std::vector<uint8_t> result={'M','T','h','d'};
    detail::be32(result,6); detail::be16(result,0); detail::be16(result,1); detail::be16(result,ppq);
    result.insert(result.end(),{'M','T','r','k'});
    detail::be32(result,uint32_t(track.size()));
    result.insert(result.end(),track.begin(),track.end());
    return result;
}

// MIDI Type 0/1 loader. If our metadata survives, all 16 steps are restored
// exactly. Otherwise, note pitch + velocity + overlaps are translated from
// ordinary MIDI, quantized to the current selected step speed.
inline Result decode(const std::vector<uint8_t>& bytes, int preferredRate=2) {
    Result r;
    if (bytes.size()<14 || bytes.size()>8*1024*1024 ||
        std::string(bytes.begin(),bytes.begin()+4)!="MThd") {
        r.error="Fichier MIDI Standard (SMF) manquant ou trop volumineux."; return r;
    }
    const uint32_t headerLen=detail::readBE(bytes,4,4);
    if(headerLen<6 || headerLen>64 || size_t(headerLen)+8>bytes.size()) { r.error="En-tete MIDI invalide."; return r; }
    const uint32_t format=detail::readBE(bytes,8,2);
    const uint32_t trackCount=detail::readBE(bytes,10,2);
    const uint32_t division=detail::readBE(bytes,12,2);
    if(format>1 || trackCount<1 || trackCount>128 || division<24 || division>=0x8000) {
        r.error="Format MIDI non pris en charge (utiliser SMF Type 0 ou 1, division PPQ)."; return r;
    }
    std::vector<detail::Note> notes;
    Pattern native;
    bool metadataFound=false;
    size_t pos=8+headerLen;
    for(uint32_t trackNo=0;trackNo<trackCount;++trackNo) {
        if(pos+8>bytes.size() || std::string(bytes.begin()+static_cast<std::ptrdiff_t>(pos),
            bytes.begin()+static_cast<std::ptrdiff_t>(pos+4))!="MTrk") {
            r.error="Piste MIDI manquante ou corrompue."; return r;
        }
        const uint32_t len=detail::readBE(bytes,pos+4,4);
        pos+=8;
        if(len>bytes.size()-pos) { r.error="Taille de piste MIDI invalide."; return r; }
        const size_t end=pos+len;
        uint8_t running=0;
        uint64_t tick=0;
        // Per-channel, per-pitch active note index; overlapping same-key notes
        // use the most recently started note, as in most mono acid patterns.
        std::array<std::array<int,128>,16> active;
        for(auto& channel:active) channel.fill(-1);
        int eventCount=0;
        while(pos<end) {
            if(++eventCount>500000) { r.error="Trop d'evenements MIDI."; return r; }
            uint32_t delta=0;
            if(!detail::takeVlq(bytes,pos,end,delta)) {r.error="Delta MIDI illisible."; return r;}
            tick+=delta;
            if(tick>1000000000ULL || pos>=end) { r.error="Temps MIDI invalide."; return r; }
            uint8_t code=bytes[pos++];
            if(code<0x80) {
                if(running==0) {r.error="Running status MIDI invalide."; return r;}
                --pos; code=running;
            } else if(code<0xf0) running=code;
            else running=0;
            if(code==0xff) {
                if(pos>=end) {r.error="Meta-evenement tronque.";return r;}
                const uint8_t type=bytes[pos++];
                uint32_t size=0;
                if(!detail::takeVlq(bytes,pos,end,size) || size>end-pos) {r.error="Meta-evenement MIDI incorrect.";return r;}
                if(type==1 && !metadataFound && size<1024) {
                    const std::string text(bytes.begin()+static_cast<std::ptrdiff_t>(pos),
                                           bytes.begin()+static_cast<std::ptrdiff_t>(pos+size));
                    metadataFound=detail::readMetadata(text,native);
                }
                pos+=size;
                if(type==0x2f) { pos=end; break; }
                continue;
            }
            if(code==0xf0 || code==0xf7) {
                uint32_t size=0;
                if(!detail::takeVlq(bytes,pos,end,size) || size>end-pos) {r.error="SysEx MIDI incorrect.";return r;}
                pos+=size; continue;
            }
            if(code>=0xf0) { r.error="Statut MIDI systeme non pris en charge."; return r; }
            const int kind=code&0xf0;
            const int channel=code&0x0f;
            const size_t count=(kind==0xc0 || kind==0xd0)?1:2;
            if(count>end-pos) {r.error="Message MIDI incomplet.";return r;}
            const uint8_t a=bytes[pos++];
            const uint8_t b=count==2?bytes[pos++]:0;
            if(a>=128 || b>=128) {r.error="Donnee MIDI hors limite.";return r;}
            if(kind==0x90 && b>0) {
                if(notes.size()>200000) {r.error="Trop de notes MIDI.";return r;}
                auto& previous=active[size_t(channel)][a];
                if(previous>=0) notes[size_t(previous)].end=uint32_t(tick);
                previous=int(notes.size());
                notes.push_back({uint32_t(tick),uint32_t(tick+1),a,b,int(trackNo)});
            } else if (kind==0x80 || (kind==0x90 && b==0)) {
                auto& previous=active[size_t(channel)][a];
                if(previous>=0) {notes[size_t(previous)].end=uint32_t(tick); previous=-1;}
            }
        }
        pos=end;
    }
    if(metadataFound) {r.ok=true;r.pattern=native;return r;}
    if(notes.empty()) {r.error="Aucune note MIDI ni metadata Acid303 reconnue.";return r;}
    // Quantize to existing step division, preserving leading rests.
    Pattern p;
    p.rate=std::clamp(preferredRate,0,3);
    p.length=16;
    const double grid=double(division)/(double(1<<p.rate));
    std::vector<int> noteAt(16,-1);
    for(size_t i=0;i<notes.size();++i) {
        const auto& n=notes[i];
        const int step=int(std::llround(double(n.start)/grid));
        if(step<0 || step>=16) continue;
        const int current=noteAt[size_t(step)];
        if(current>=0 && notes[size_t(current)].velocity>=n.velocity) continue;
        noteAt[size_t(step)]=int(i);
        auto& s=p.steps[size_t(step)];
        s.on=true; s.note=n.pitch; s.accent=n.velocity>=100;
    }
    for(int i=1;i<16;++i) {
        const int cur=noteAt[size_t(i)];
        if(cur<0) continue;
        const double at=double(notes[size_t(cur)].start);
        for(int j=i-1;j>=0;--j) {
            const int prior=noteAt[size_t(j)];
            if(prior>=0) {
                if(double(notes[size_t(prior)].end)>at + 0.1)
                    p.steps[size_t(i)].slide=true;
                break;
            }
        }
    }
    p.nativeMetadata=false;
    r.ok=true;r.pattern=p;
    return r;
}
} // namespace acid303::midi
