#include "../Source/MidiPattern.h"
#include <cassert>
#include <cstdint>
#include <iostream>
#include <vector>
#include <algorithm>
using namespace acid303::midi;

int main() {
    for (int rate=0;rate<4;++rate) {
        for (int length=1;length<=16;++length) {
            Pattern input;
            input.rate=rate;input.length=length;
            for(int i=0;i<16;++i) {
                input.steps[size_t(i)]={i%4 != 1, uint8_t(36+i), i%3 == 0, i%4 == 0};
            }
            auto encoded=encode(input);
            auto decoded=decode(encoded);
            assert(decoded.ok);
            assert(decoded.pattern.nativeMetadata);
            assert(decoded.pattern.rate==rate);
            assert(decoded.pattern.length==length);
            for(int i=0;i<16;++i)
                assert(acid303::seq::pack(decoded.pattern.steps[size_t(i)])==
                       acid303::seq::pack(input.steps[size_t(i)]));
        }
    }
    // Fall back to MIDI notes + velocity/overlap when Acid303 metadata is
    // absent (e.g. a file exported/edited in FL Studio).
    Pattern pattern;
    pattern.rate=2;pattern.length=4;
    pattern.steps[0]={true,48,false,true};
    pattern.steps[1]={true,50,true,false};
    pattern.steps[2]={false,50,false,false};
    pattern.steps[3]={true,53,false,false};
    const auto raw=encode(pattern);
    auto withoutMeta=raw;
    const std::string magic="ACID303_PATTERN_V1";
    auto text=std::search(withoutMeta.begin(),withoutMeta.end(),magic.begin(),magic.end());
    // Overwrite only the recognizable marker; event framing is unchanged.
    if(text!=withoutMeta.end()) *text='X';
    const auto fallback=decode(withoutMeta,2);
    assert(fallback.ok && !fallback.pattern.nativeMetadata);
    assert(fallback.pattern.steps[0].on && fallback.pattern.steps[0].accent);
    assert(fallback.pattern.steps[1].on && fallback.pattern.steps[1].slide);
    assert(!fallback.pattern.steps[2].on);
    assert(fallback.pattern.steps[3].on);
    // Failing safely on invalid content
    auto broken=raw;broken[0]='B';
    assert(!decode(broken).ok);
    std::cout << "MIDI: 64 native round-trips, external fallbacks, errors OK\n";
}
