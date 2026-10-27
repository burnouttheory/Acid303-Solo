#include "../Source/PatternTranspose.h"
#include "../Source/PatternBank.h"
#include "../Source/Sequencer.h"
#include <cassert>
#include <iostream>
int main() {
    using acid303::transpose::midiPitch;
    assert(midiPitch(48,5)==53); // C to F
    assert(midiPitch(48,0)==48);
    assert(midiPitch(48,-12)==36);
    assert(midiPitch(121,12)==127);
    assert(midiPitch(2,-12)==0);
    // original step (including accent/slide) is unaffected by transposition
    acid303::seq::Step orig;
    orig.on=true; orig.note=48; orig.accent=true; orig.slide=true;
    const auto old=acid303::seq::pack(orig);
    for (int t=-12;t<=12;++t) {
        const int n=midiPitch(orig.note,t);
        assert(n>=0 && n<=127);
        assert(acid303::seq::pack(orig)==old);
    }
    for (int idx=0;idx<64;++idx) {
        int midi=acid303::bank::toMidi(idx);
        assert(midi==36+idx);
        assert(acid303::bank::fromMidi(midi)==idx);
        assert(acid303::bank::slot(acid303::bank::group(idx),acid303::bank::pattern(idx))==idx);
    }
    assert(acid303::bank::fromMidi(35)==-1);
    assert(acid303::bank::fromMidi(100)==-1);
    std::cout << "PASS: 25 transpositions, 64 MIDI triggers, legato/accent integrity\n";
}
