#!/usr/bin/env python3
"""Regressions targeted at the two MSVC build errors reported by FL Studio tester."""
from pathlib import Path
import re, hashlib
root=Path(__file__).resolve().parents[1]
ed=(root/'Source/PluginEditor.cpp').read_text('utf-8')
proc=(root/'Source/PluginProcessor.cpp').read_text('utf-8')
# Find the specific timer callback body only.
timer=ed.split('void Acid303SoloEditor::timerCallback()',1)[1].split('void Acid303SoloEditor::ensureFactorySkinFiles()',1)[0]
assert len(re.findall(r'\bconst\s+int\s+selected\s*=',timer))==1
assert 'const int selectedPattern=processor.getSelectedPattern();' in timer
assert 'selectedPattern/16+1' in timer
# Import targets the selected bank and not an obsolete variable named steps.
imp=proc.split('juce::Result Acid303SoloProcessor::importMidiFromFile(',1)[1].split('juce::AudioProcessorEditor*',1)[0]
assert 'const auto targetPattern = static_cast<size_t>(getSelectedPattern());' in imp
assert 'bankSteps[targetPattern]' in imp
assert not re.search(r'(?<![\w.])steps\s*\[',imp)
assert 'setHostParameter("seqLength", result.pattern.length);' in imp
assert 'bankTriggerMode' in proc
assert 'PATTERN_BANKS_V3' in proc
assert 'Acid303 Solo.vst3' in (root/'Installeur/Acid303Solo.iss').read_text('utf-8')
assert hashlib.sha256((root/'Source/Engine.h').read_bytes()).hexdigest()=='7cfb55fffeeb86f7eb65df982e78e68b1f485857a7875813092117784c5a0236'
print('PASS: selected variable, MIDI import selected bank, upgrade identity and identical V2.0.6 audio engine')
