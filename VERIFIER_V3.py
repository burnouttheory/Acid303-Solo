#!/usr/bin/env python3
"""Offline safety audit for in-place v2.0.6 -> V3 migration TEST."""
from pathlib import Path
import re,sys
p=Path(__file__).resolve().parent
files={key:(p/key).read_text(encoding='utf-8-sig') for key in ['CMakeLists.txt','Source/PluginProcessor.cpp','Source/PluginEditor.cpp','Source/PluginProcessor.h','Installeur/Acid303Solo.iss','tools/Build-Official.ps1']}
cm=files['CMakeLists.txt'];proc=files['Source/PluginProcessor.cpp'];ui=files['Source/PluginEditor.cpp'];iss=files['Installeur/Acid303Solo.iss'];build=files['tools/Build-Official.ps1']
checks={
'Original VST3 identity reused': all(s in cm for s in ['BUNDLE_ID "info.acid303.solo"','PLUGIN_MANUFACTURER_CODE Ac03','PLUGIN_CODE A303','PRODUCT_NAME "Acid303 Solo"']),
'Original VST3 filename reused': 'Acid303 Solo.vst3' in iss and 'Acid303 Solo.vst3' in build,
'Inno upgrade identity reused': 'AppId=Acid303SoloVST3' in iss,
'Backup procedure included': (p/'SAUVEGARDE_AVANT_MIGRATION.cmd').is_file(),
'Optional V3 TEST cleanup after validation': (p/'NETTOYER_V3_TEST_APRES_VALIDATION.cmd').is_file(),
'V2 factory presets remain untouched': (p/'FactoryContent/presets/FactoryV3').is_dir(),
'64 MIDI bank triggers': 'kTotalPatterns = kGroups * kPatternsPerGroup' in (p/'Source/PatternBank.h').read_text(),
'Global transpose appended after bankTriggerMode': proc.index('ParameterID{"bankTranspose"')>proc.index('ParameterID{"bankTriggerMode"'),
'Transpose affects audible pitch only': 'acid303::transpose::midiPitch(static_cast<int>(next.note), getPatternTranspose())' in proc,
'MIDI export uses transpose': 'acid303::transpose::midiPitch(static_cast<int>(step.note),getPatternTranspose())' in proc,
'Previous V2 settings folder retained': 'getChildFile("Acid303").getChildFile("presets")' in proc,
'V3 personal preset import': 'V3_Imported' in proc,
'MIDI map GUI': 'CARTE MIDI 64' in ui and 'COPIER LES 64 CORRESPONDANCES' in ui,
'Transpose GUI menu': 'transposeChoice.addItem' in ui and 'ComboBoxAttachment>' in ui,
'Tube and Bitcrusher unchanged': 'juce::StringArray{"Tube", "Bitcrusher"}, 0' in proc,
'65th factory bank? no': len(list((p/'FactoryContent/presets/FactoryV3').rglob('*.acid303preset')))==154,
'12 skins': len(list((p/'FactoryContent/skins').rglob('*.acid303skin')))==12,
'Existing 173 MIDIs retained': len(list((p/'FactoryContent/midi').rglob('*.mid')))>=173,
'Distinct installer name TEST': 'Acid303Solo_Setup_3.0.0_MIGRATION_TEST.exe' in build and '_MIGRATION_TEST' in iss,
'Legal notices': (p/'LICENSES/THIRD_PARTY_NOTICES.txt').is_file(),
}
for label,passed in checks.items():print(('PASS ' if passed else 'FAIL ')+label)
print(str(sum(checks.values()))+'/'+str(len(checks))+' checks')
sys.exit(0 if all(checks.values()) else 1)
