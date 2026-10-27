from pathlib import Path
import re
root=Path(__file__).resolve().parents[1]
cpp=(root/'Source/PluginProcessor.cpp').read_text()
ui=(root/'Source/PluginEditor.cpp').read_text()
engine=(root/'Source/Engine.h').read_text()
assert 'juce::StringArray{"Tube", "Bitcrusher"}, 0' in cpp
assert 'juce::ParameterID{"distType",1}' in cpp
assert 'distAcidMode' not in cpp+ui+engine
assert 'std::array<juce::String,2> typeNames' in ui
assert 'std::clamp(c.distType, 0, 1)' in engine
assert 'case 0: { // TUBE name retained' in engine
assert 'case 1: { // Bitcrusher:' in engine
print('PASS: existing 2-choice parameter, automation ID, GUI and DSP mode order retained')
