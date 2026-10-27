from pathlib import Path
import re
root = Path(__file__).resolve().parents[1]
src = (root/'Source/PluginEditor.cpp').read_text(encoding='utf-8')
# Both skins render the brand in the same distinct top-right slot.
branding = re.findall(r'g\.drawText\("BURNOUT THEORY",\s*(\d+),\s*(\d+),\s*(\d+),\s*(\d+),', src)
assert branding == [('1210', '23', '210', '25')]*2, branding
left, top, width, height = map(int, branding[0])
assert left >= 1201 and left+width <= 1440
assert top >= 0 and top+height < 68  # Pattern Bank starts at y=68
# Existing upper controls have independent bounds in the current V3 editor.
for ctrl in ('languageChoice', 'tempoLabel', 'skinLabel', 'skinChoice'):
    match = re.search(rf'{ctrl}\.setBounds\(B\((\d+),(\d+),(\d+),(\d+)\)\)', src)
    assert match, ctrl
    x, y, w, h = map(int, match.groups())
    assert x + w < left or left + width < x or y + h < top or top + height < y, ctrl
assert 'std::pair<const char*,const char*>,28>' in src
assert 'patternGroupKnob.setBounds' in src and 'patternNumberKnob.setBounds' in src
assert 'startTimerHz(20)' in src
print('PASS: brand outside French/tempo/skin controls and above Pattern Bank; V3 controls preserved')
