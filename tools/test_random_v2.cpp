#include "../Source/RandomPattern.h"
#include <cassert>
#include <set>
#include <iostream>
int main() {
    std::set<std::array<unsigned int,16>> patterns;
    for (unsigned int seed=0;seed<200;seed++) {
        const auto a=acid303::seq::generateAcidPattern(seed);
        const auto same=acid303::seq::generateAcidPattern(seed);
        for (int j=0;j<16;++j) assert(acid303::seq::pack(a[j])==acid303::seq::pack(same[j]));
        int on=0;
        std::array<unsigned int,16> signature{};
        for(int i=0;i<16;i++) {
            const auto& s=a[i];
            on+=s.on;
            assert(s.note>=36&&s.note<=67);
            assert(!s.slide || (i>0 && s.on && a[i-1].on));
            assert(!s.accent || s.on);
            const auto encoded=acid303::seq::pack(s);
            const auto decoded=acid303::seq::unpack(encoded);
            assert(decoded.on==s.on && decoded.note==s.note && decoded.slide==s.slide && decoded.accent==s.accent);
            signature[i]=encoded;
        }
        assert(on>=6);
        patterns.insert(signature);
    }
    assert(patterns.size()>190);
    std::cout << "OK: 200 random patterns, deterministic, 16 steps, density/pitch/slide/accent, "
              << patterns.size() << " unique patterns\n";
}
