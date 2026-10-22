#include "PluginEditor.h"
#include <array>
#include <algorithm>
#include <cmath>
#include <set>

namespace {
constexpr float virtualWidth = 1200.0f;
constexpr float virtualHeight = 900.0f;
const juce::Colour blackMetal(0xff030506);
const juce::Colour neonGreen(0xff4cff72);
const juce::Colour neonGreenSoft(0xff97ffab);
const juce::Colour silver1(0xffeef2f4);
const juce::Colour silver2(0xffc8d2d8);
const juce::Colour silver3(0xff78848c);

struct SkinTheme {
    juce::String name;
    juce::Colour accent{0xff4cff72};
    juce::Colour soft{0xff97ffab};
    juce::Colour body{0xff030506};
    juce::Colour deep{0xff010203};
    juce::Colour panel{0xff070b0d};
    juce::Colour crackA{0xff16221a};
    juce::Colour crackB{0xff0a120d};
    juce::Colour fxAccent{0xffd22a48};
    juce::Colour fxGlow{0xff6c101d};
    juce::Colour dim{0xff42604a};
};

juce::String defaultSkinName() { return "V0.25 NEON GREEN"; }

SkinTheme makeThemeV25() {
    SkinTheme t; t.name = "V0.25 NEON GREEN";
    t.accent = juce::Colour(0xff4cff72); t.soft = juce::Colour(0xff97ffab);
    t.body = juce::Colour(0xff030506); t.deep = juce::Colour(0xff010203);
    t.panel = juce::Colour(0xff070b0d); t.crackA = juce::Colour(0xff16221a); t.crackB = juce::Colour(0xff0a120d);
    t.fxAccent = juce::Colour(0xffd22a48); t.fxGlow = juce::Colour(0xff6c101d); t.dim = juce::Colour(0xff42604a);
    return t;
}
SkinTheme makeThemeV26() {
    SkinTheme t; t.name = "V0.26 DARK GRID";
    t.accent = juce::Colour(0xff7cff9a); t.soft = juce::Colour(0xffc9ffd4);
    t.body = juce::Colour(0xff05080a); t.deep = juce::Colour(0xff020304);
    t.panel = juce::Colour(0xff0a0f12); t.crackA = juce::Colour(0xff1c2d24); t.crackB = juce::Colour(0xff101813);
    t.fxAccent = juce::Colour(0xffe63758); t.fxGlow = juce::Colour(0xff742033); t.dim = juce::Colour(0xff53725b);
    return t;
}
SkinTheme makeThemeV24() {
    SkinTheme t; t.name = "V0.24 CLASSIC";
    t.accent = juce::Colour(0xffd4f4d9); t.soft = juce::Colour(0xffeef9f1);
    t.body = juce::Colour(0xff121719); t.deep = juce::Colour(0xff090b0c);
    t.panel = juce::Colour(0xff20282b); t.crackA = juce::Colour(0xff172025); t.crackB = juce::Colour(0xff0c1012);
    t.fxAccent = juce::Colour(0xffcf3f54); t.fxGlow = juce::Colour(0xff66212a); t.dim = juce::Colour(0xff7a8d80);
    return t;
}
SkinTheme makeThemeFire() {
    SkinTheme t; t.name = "FIRE";
    t.accent = juce::Colour(0xffff7a1a); t.soft = juce::Colour(0xffffd18c);
    t.body = juce::Colour(0xff140302); t.deep = juce::Colour(0xff070101);
    t.panel = juce::Colour(0xff1d0806); t.crackA = juce::Colour(0xff4d130a); t.crackB = juce::Colour(0xff230604);
    t.fxAccent = juce::Colour(0xffff3d19); t.fxGlow = juce::Colour(0xff7b1708); t.dim = juce::Colour(0xff84513d);
    return t;
}
std::vector<SkinTheme> factoryThemes() {
    return { makeThemeV25(), makeThemeV26(), makeThemeV24(), makeThemeFire() };
}

juce::String colourHex(juce::Colour c) {
    return juce::String::toHexString((int)c.getARGB()).paddedLeft('0', 8).toUpperCase();
}

juce::String skinFileName(const juce::String& displayName) {
    auto n = displayName;
    n = n.replaceCharacter(' ', '_');
    n = n.replaceCharacter('.', '_');
    return n + ".acid303skin";
}

void writeSkinFile(const SkinTheme& t, const juce::File& file) {
    juce::XmlElement xml("ACID303SKIN");
    xml.setAttribute("name", t.name);
    xml.setAttribute("accent", colourHex(t.accent));
    xml.setAttribute("soft", colourHex(t.soft));
    xml.setAttribute("body", colourHex(t.body));
    xml.setAttribute("deep", colourHex(t.deep));
    xml.setAttribute("panel", colourHex(t.panel));
    xml.setAttribute("crackA", colourHex(t.crackA));
    xml.setAttribute("crackB", colourHex(t.crackB));
    xml.setAttribute("fxAccent", colourHex(t.fxAccent));
    xml.setAttribute("fxGlow", colourHex(t.fxGlow));
    xml.setAttribute("dim", colourHex(t.dim));
    file.replaceWithText(xml.toString());
}

juce::Colour parseHexColour(const juce::String& value, juce::Colour fallback) {
    auto txt = value.trim().toUpperCase();
    txt = txt.retainCharacters("0123456789ABCDEF");
    if (txt.length() == 6) txt = "FF" + txt;
    if (txt.length() != 8) return fallback;
    return juce::Colour((juce::uint32) txt.getHexValue64());
}

bool loadSkinFile(const juce::File& file, SkinTheme& out) {
    if (!file.existsAsFile()) return false;
    std::unique_ptr<juce::XmlElement> xml(juce::XmlDocument::parse(file));
    if (xml == nullptr) {
        // Older B&W / W&B skins contained a raw ampersand in the XML name
        // attribute. Repair it IN MEMORY (never edit the user's file).
        auto content = file.loadFileAsString();
        const int marker = content.indexOf("name=\"");
        if (marker >= 0) {
            const int start = marker + 6;
            const int finish = content.indexOfChar(start, '"');
            if (finish > start) {
                auto name = content.substring(start, finish);
                name = name.replace("&amp;", "__ACID_AMP_ENTITY__")
                           .replace("&", "&amp;")
                           .replace("__ACID_AMP_ENTITY__", "&amp;");
                content = content.substring(0, start) + name + content.substring(finish);
                xml = juce::XmlDocument::parse(content);
            }
        }
    }
    if (xml == nullptr || !xml->hasTagName("ACID303SKIN")) return false;
    out.name = xml->getStringAttribute("name", file.getFileNameWithoutExtension());
    out.accent = parseHexColour(xml->getStringAttribute("accent"), out.accent);
    out.soft = parseHexColour(xml->getStringAttribute("soft"), out.soft);
    out.body = parseHexColour(xml->getStringAttribute("body"), out.body);
    out.deep = parseHexColour(xml->getStringAttribute("deep"), out.deep);
    out.panel = parseHexColour(xml->getStringAttribute("panel"), out.panel);
    out.crackA = parseHexColour(xml->getStringAttribute("crackA"), out.crackA);
    out.crackB = parseHexColour(xml->getStringAttribute("crackB"), out.crackB);
    out.fxAccent = parseHexColour(xml->getStringAttribute("fxAccent"), out.fxAccent);
    out.fxGlow = parseHexColour(xml->getStringAttribute("fxGlow"), out.fxGlow);
    out.dim = parseHexColour(xml->getStringAttribute("dim"), out.dim);
    return true;
}

SkinTheme resolveSkinThemeByName(const juce::String& requestedName) {
    for (const auto& theme : factoryThemes()) if (theme.name == requestedName) return theme;
    const auto folder = Acid303SoloProcessor::getUserSkinFolder();
    const auto exact = folder.getChildFile(skinFileName(requestedName));
    SkinTheme t;
    if (loadSkinFile(exact, t)) return t;
    for (const auto& f : folder.findChildFiles(juce::File::findFiles, false, "*.acid303skin")) {
        SkinTheme candidate;
        if (loadSkinFile(f, candidate) && candidate.name == requestedName) return candidate;
    }
    return makeThemeV25();
}

juce::String getStoredSkinName(const Acid303SoloProcessor& p) {
    auto n = p.params.state.getProperty(juce::Identifier("selectedSkinName"), juce::var()).toString().trim();
    if (n.isNotEmpty()) return n;
    // Backward compatibility: old uiSkin parameter only knew Green / Blue.
    const auto* raw = p.params.getRawParameterValue("uiSkin");
    if (raw != nullptr && raw->load(std::memory_order_relaxed) >= 0.5f)
        return juce::String("V0.26 DARK GRID");
    return defaultSkinName();
}

void keepHostKeyboardFocus(juce::Component& component) {
    component.setWantsKeyboardFocus(false);
    component.setMouseClickGrabsKeyboardFocus(false);
}

void drawCrackTexture(juce::Graphics& g, juce::Rectangle<float> area, const SkinTheme& theme) {
    g.saveState();
    g.reduceClipRegion(area.toNearestInt());
    for (int i = 0; i < 85; ++i) {
        const float seed = float(i * 37 + 11);
        const float x1 = area.getX() + std::fmod(seed * 19.7f, area.getWidth());
        const float y1 = area.getY() + std::fmod(seed * 13.1f, area.getHeight());
        const float dx = std::sin(seed * 0.51f) * (14.0f + std::fmod(seed, 27.0f));
        const float dy = std::cos(seed * 0.37f) * (11.0f + std::fmod(seed, 33.0f));
        g.setColour(theme.crackA.withAlpha(0.35f));
        g.drawLine(x1, y1, x1 + dx, y1 + dy, 0.85f);
        g.setColour(theme.crackB.withAlpha(0.85f));
        g.drawLine(x1 + dx, y1 + dy, x1 + dx * 1.22f, y1 + dy * 1.22f, 0.55f);
    }
    g.restoreState();
}

void panel(juce::Graphics& g, juce::Rectangle<float> r, const juce::String& title,
           const juce::String& sub, bool redMode, const SkinTheme& theme) {
    g.setColour(juce::Colours::black.withAlpha(0.6f));
    g.fillRoundedRectangle(r.translated(2.0f,4.0f), 7.0f);
    g.setColour(theme.panel);
    g.fillRoundedRectangle(r, 7.0f);
    g.setColour(redMode ? theme.fxAccent.withAlpha(0.93f) : theme.accent.withAlpha(0.88f));
    g.drawRoundedRectangle(r.reduced(0.8f), 6.0f, 1.7f);
    g.setColour(redMode ? theme.fxGlow.withAlpha(0.28f) : theme.accent.withAlpha(0.14f));
    g.fillRoundedRectangle(r.reduced(1.8f), 5.7f);
    const auto header=juce::Rectangle<float>(r.getX()+9,r.getY()+8,r.getWidth()-18,29);
    const bool lightTheme = theme.panel.getPerceivedBrightness() > 0.65f;
    g.setColour(lightTheme ? theme.body.darker(0.04f)
                           : (redMode ? juce::Colour(0xff180306) : juce::Colour(0xff031108)));
    g.fillRoundedRectangle(header,3.5f);
    g.setColour(redMode ? theme.fxAccent : theme.accent);
    g.fillEllipse(header.getX()+8,header.getY()+8,11,11);
    g.setColour(redMode ? theme.fxAccent : theme.accent);
    g.setFont(juce::Font(juce::FontOptions(16.0f).withStyle("Bold")));
    g.drawText(title,header.withTrimmedLeft(25).reduced(0,2),juce::Justification::centredLeft);
    g.setColour(redMode ? theme.fxAccent.brighter(0.5f) : theme.soft);
    g.setFont(juce::Font(juce::FontOptions(10.0f).withStyle("Bold")));
    g.drawText(sub,header.reduced(14,2),juce::Justification::centredRight);
}
}


void VintageKnobLook::drawRotarySlider(juce::Graphics& g, int x, int y, int w, int h,
                                        float pos, float start, float end, juce::Slider&) {
    const auto area=juce::Rectangle<float>((float)x,(float)y,(float)w,(float)h).reduced(4.0f);
    const auto cx=area.getCentreX();
    const auto cy=area.getCentreY();
    const float radius=std::min(area.getWidth(),area.getHeight())*0.39f;
    const float a=start+pos*(end-start);
    for (int i=0;i<=12;++i) {
        const float t=start+(end-start)*(float(i)/12.0f);
        const float sx=cx+std::sin(t)*(radius+6.0f), sy=cy-std::cos(t)*(radius+6.0f);
        const float ex=cx+std::sin(t)*(radius+12.0f),ey=cy-std::cos(t)*(radius+12.0f);
        g.setColour(accentColour.withAlpha(i % 3 == 0 ? 0.95f : 0.55f));
        g.drawLine(sx,sy,ex,ey,i%3==0?1.8f:1.0f);
    }
    g.setColour(juce::Colours::black.withAlpha(0.42f));
    g.fillEllipse(cx-radius+1,cy-radius+3,2*radius,2*radius);
    g.setGradientFill(juce::ColourGradient(silver1,cx-radius*0.8f,cy-radius,
                       silver3,cx+radius,cy+radius,false));
    g.fillEllipse(cx-radius,cy-radius,2*radius,2*radius);
    g.setColour(silver2.withAlpha(0.95f));
    g.drawEllipse(cx-radius+1,cy-radius+1,2*radius-2,2*radius-2,1.2f);
    g.setColour(juce::Colour(0xff444b52));
    g.fillEllipse(cx-radius*0.72f,cy-radius*0.72f,1.44f*radius,1.44f*radius);
    g.setColour(juce::Colours::white.withAlpha(0.18f));
    g.fillEllipse(cx-radius*0.45f,cy-radius*0.68f,radius*0.58f,radius*0.24f);
    const float tip=radius*0.77f;
    g.setColour(accentColour);
    g.drawLine(cx+std::sin(a)*radius*0.22f,cy-std::cos(a)*radius*0.22f,
               cx+std::sin(a)*tip,cy-std::cos(a)*tip,3.6f);
    g.setColour(accentColour.brighter(0.4f).withAlpha(0.75f));
    g.fillEllipse(cx-3.0f, cy-3.0f, 6.0f, 6.0f);
}

Acid303SoloEditor::Acid303SoloEditor(Acid303SoloProcessor& proc)
    : AudioProcessorEditor(&proc), processor(proc), sequenceGrid(proc) {
    keepHostKeyboardFocus(*this);
    keepHostKeyboardFocus(sequenceGrid);
    // Explicit base pointer type is necessary: these controls are a mixture of
    // juce::TextButton, juce::ComboBox and juce::Label. MSVC cannot deduce a
    // common initializer_list type from different derived pointer classes.
    const std::array<juce::Component*, 21> keyboardFocusControls{{
        &modeButton, &rateChoice, &rateLabel,
        &savePresetButton, &loadPresetButton, &presetFolderButton, &updateButton,
        &previousPresetButton, &presetNameLabel, &nextPresetButton, &skinLabel, &skinChoice,
        &exportMidiButton, &importMidiButton, &midiFolderButton,
        &delayModeChoice, &delaySubdivision,
        &prevDistType, &nextDistType, &distTypeName, &tempoLabel
    }};
    for (auto* control : keyboardFocusControls)
        keepHostKeyboardFocus(*control);
    const std::array<std::pair<const char*,const char*>,28> names = {{
        {"wave","WAVE"}, {"shape","SHAPE"}, {"tune","TUNE"},
        {"cutoff","CUTOFF"}, {"mod","MOD"}, {"reso","RESO"}, {"lfoRate","LFO RATE"},
        {"envMod","ENV MOD"}, {"slideTime","SLIDE TIME"}, {"decay","DECAY"},
        {"accent","ACCENT"}, {"masterVolume","VOLUME GENERAL"},
        {"distDrive","DRIVE"}, {"distVolume","LEVEL"}, {"distEq","EQ"},
        {"delayTime","TIME"}, {"delayFeedback","FEEDBACK"},
        {"reverbSize","SIZE"}, {"reverbVolume","VOLUME"},
        {"distWet","DRY/WET"}, {"delayWet","DRY/WET"}, {"reverbWet","DRY/WET"},
        {"compLow","LOW"}, {"compMid","MID"}, {"compHigh","HIGH"},
        {"compGain","GAIN"}, {"compWet","DRY/WET"},
        {"delayWidth","WIDTH"}
    }};
    for (const auto& item : names) addControl(item.first,item.second);
    addEffectButton("compOn");
    addEffectButton("distOn");
    addEffectButton("delayOn");
    addEffectButton("reverbOn");
    for (auto* button : { &prevDistType, &nextDistType }) {
        button->setColour(juce::TextButton::buttonColourId, blackMetal);
        button->setColour(juce::TextButton::textColourOffId, neonGreen);
        addAndMakeVisible(*button);
    }
    prevDistType.setTooltip("Previous distortion type");
    nextDistType.setTooltip("Next distortion type");
    prevDistType.onClick = [this] { stepDistType(-1); };
    nextDistType.onClick = [this] { stepDistType(1); };
    distTypeName.setJustificationType(juce::Justification::centred);
    distTypeName.setColour(juce::Label::backgroundColourId, juce::Colour(0xff07130b));
    distTypeName.setColour(juce::Label::textColourId, neonGreen);
    distTypeName.setFont(juce::Font(juce::FontOptions(13.0f).withStyle("Bold")));
    distTypeName.setTooltip("Type de distortion. Les fleches changent le parametre VST3 automatisable.");
    addAndMakeVisible(distTypeName);
    tempoLabel.setJustificationType(juce::Justification::centredRight);
    tempoLabel.setColour(juce::Label::textColourId,neonGreen);
    tempoLabel.setFont(juce::Font(juce::FontOptions(15.0f).withStyle("Bold")));
    addAndMakeVisible(tempoLabel);
    delaySubdivision.setJustificationType(juce::Justification::centred);
    delaySubdivision.setColour(juce::Label::textColourId,neonGreenSoft);
    delaySubdivision.setFont(juce::Font(juce::FontOptions(10.5f).withStyle("Bold")));
    addAndMakeVisible(delaySubdivision);
    delayModeChoice.addItem("NORMAL", 1);
    delayModeChoice.addItem("PING-PONG", 2);
    delayModeChoice.setColour(juce::ComboBox::backgroundColourId, blackMetal);
    delayModeChoice.setColour(juce::ComboBox::textColourId, neonGreen);
    delayModeChoice.setColour(juce::ComboBox::outlineColourId, neonGreen);
    delayModeChoice.setTooltip("NORMAL : repetition classique; PING-PONG : alternance gauche / droite. Reglage automatisable.");
    addAndMakeVisible(delayModeChoice);
    delayModeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        processor.params, "delayMode", delayModeChoice);
    addAndMakeVisible(sequenceGrid);
    modeButton.setColour(juce::TextButton::buttonColourId, blackMetal);
    modeButton.setColour(juce::TextButton::textColourOffId, neonGreen);
    modeButton.setTooltip("Piano Roll : jouez les notes MIDI. Sequenceur : une note MIDI maintenue declenche les 16 pas, note relachee = stop. Tempo FL Studio.");
    modeButton.onClick = [this] {
        processor.setPlayMode(1 - processor.getPlayMode());
        timerCallback();
    };
    addAndMakeVisible(modeButton);
    rateLabel.setText("VITESSE", juce::dontSendNotification);
    rateLabel.setColour(juce::Label::textColourId, neonGreen);
    rateLabel.setFont(juce::Font(juce::FontOptions(12.0f).withStyle("Bold")));
    rateLabel.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(rateLabel);
    rateChoice.addItem("NOIRE (1/4)", 1);
    rateChoice.addItem("CROCHE (1/8)", 2);
    rateChoice.addItem("DOUBLE CROCHE (1/16)", 3);
    rateChoice.addItem("TRIPLE CROCHE (1/32)", 4);
    rateChoice.setColour(juce::ComboBox::backgroundColourId, blackMetal);
    rateChoice.setColour(juce::ComboBox::textColourId, neonGreen);
    rateChoice.setColour(juce::ComboBox::outlineColourId, neonGreen);
    rateChoice.setTooltip("Vitesse de lecture de chaque pas. Toujours synchronisee avec FL Studio.");
    addAndMakeVisible(rateChoice);
    rateAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        processor.params, "seqRate", rateChoice);
    // Upper patch-management strip: previous/name/next plus SAVE/LOAD/FOLDER.
    // Skin selector lives in the top right beside the FL Studio BPM display.
    for (auto* arrow : { &previousPresetButton, &nextPresetButton }) {
        arrow->setColour(juce::TextButton::buttonColourId, juce::Colour(0xff05110a));
        arrow->setColour(juce::TextButton::textColourOffId, neonGreen);
        addAndMakeVisible(*arrow);
    }
    previousPresetButton.setTooltip("Charger le preset precedent de Documents/Acid303/presets");
    nextPresetButton.setTooltip("Charger le preset suivant de Documents/Acid303/presets");
    previousPresetButton.onClick = [this] { stepUserPreset(-1); };
    nextPresetButton.onClick = [this] { stepUserPreset(1); };
    presetNameLabel.setJustificationType(juce::Justification::centredLeft);
    presetNameLabel.setFont(juce::Font(juce::FontOptions(13.0f).withStyle("Bold")));
    presetNameLabel.setTooltip("Nom du preset enregistre ou charge; sauvegarde dans Documents/Acid303/presets");
    addAndMakeVisible(presetNameLabel);
    skinLabel.setText("SKINS", juce::dontSendNotification);
    skinLabel.setFont(juce::Font(juce::FontOptions(12.0f).withStyle("Bold")));
    skinLabel.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(skinLabel);
    skinChoice.setTooltip("Choisir un habillage visuel depuis Documents/Acid303/skins");
    addAndMakeVisible(skinChoice);
    for (auto* button : {&savePresetButton, &loadPresetButton, &presetFolderButton}) {
        button->setColour(juce::TextButton::buttonColourId, juce::Colour(0xff05110a));
        button->setColour(juce::TextButton::textColourOffId, neonGreen);
        addAndMakeVisible(*button);
    }
    savePresetButton.setTooltip("Enregistrer tous les parametres et les 16 pas (.acid303preset)");
    loadPresetButton.setTooltip("Recharger un preset Acid303 (.acid303preset)");
    presetFolderButton.setTooltip("Ouvrir Documents/Acid303/presets dans Explorateur Windows");
    savePresetButton.onClick = [this] { choosePresetFile(true); };
    loadPresetButton.onClick = [this] { choosePresetFile(false); };
    presetFolderButton.onClick = [] {
        auto result = Acid303SoloProcessor::preparePresetFolder();
        if (result.failed()) {
            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                "Dossier des presets", result.getErrorMessage());
        } else {
            Acid303SoloProcessor::getUserPresetFolder().revealToUser();
        }
    };
    updateButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff06110a));
    updateButton.setColour(juce::TextButton::textColourOffId, neonGreen);
    updateButton.setTooltip("Verifier les versions publiees sur GitHub, puis ouvrir la page de telechargement.");
    updateButton.onClick = [this] { handleUpdateClick(); };
    addAndMakeVisible(updateButton);
    for (auto* button : {&exportMidiButton, &importMidiButton, &midiFolderButton}) {
        button->setColour(juce::TextButton::buttonColourId, juce::Colour(0xff05110a));
        button->setColour(juce::TextButton::textColourOffId, neonGreen);
        addAndMakeVisible(*button);
    }
    exportMidiButton.setTooltip("Exporter les 16 pas en .mid (velocity=accent, overlap=slide, metadonnees Acid303)");
    importMidiButton.setTooltip("Importer les notes MIDI d'un fichier .mid et reconstituer la sequence");
    midiFolderButton.setTooltip("Ouvrir le dossier Documents/Acid303/midi");
    exportMidiButton.onClick = [this] { chooseMidiFile(true); };
    importMidiButton.onClick = [this] { chooseMidiFile(false); };
    midiFolderButton.onClick = [] {
        const auto result = Acid303SoloProcessor::prepareMidiFolder();
        if (result.failed())
            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                "Dossier MIDI", result.getErrorMessage());
        else Acid303SoloProcessor::getUserMidiFolder().revealToUser();
    };
    ensureFactorySkinFiles();
    refreshSkinChoices();
    processor.params.state.setProperty(juce::Identifier("selectedSkinName"), getStoredSkinName(processor), nullptr);
    skinChoice.onChange = [this] {
        const int idx = skinChoice.getSelectedItemIndex();
        if (idx >= 0 && idx < availableSkinNames.size()) {
            const auto chosen = availableSkinNames[idx];
            processor.params.state.setProperty(juce::Identifier("selectedSkinName"), chosen, nullptr);
            applySkin(chosen);
        }
    };
    // Compact opening size, live resize handle at bottom-right.
    // Components and the artwork use the same 1200x900 reference canvas.
    setResizable(true, true);
    setResizeLimits(900, 675, 1560, 1170);
    setSize(960, 720);
    applySkin(getStoredSkinName(processor));
    checkForUpdates();
    startTimerHz(8);
    timerCallback();
}
Acid303SoloEditor::~Acid303SoloEditor() {
    updateWorker.signalThreadShouldExit();
    updateWorker.stopThread(6000);
    for (auto& c : controls) if (c.slider) c.slider->setLookAndFeel(nullptr);
}
void Acid303SoloEditor::addControl(const char* id, const char* title) {
    Control c;
    c.slider=std::make_unique<AutomationSlider>();
    keepHostKeyboardFocus(*c.slider);
    // Ask the VST3 host for its parameter menu on right-click.
    // This uses the REAL associated APVTS parameter, not a fake UI menu entry.
    // In FL Studio the host-provided menu may include Create automation clip.
    auto* parameter = processor.params.getParameter(id);
    c.slider->onRightClick = [this, parameter](const juce::MouseEvent& event) {
        if (auto* host = getHostContext()) {
            if (auto provided = host->getContextMenuForParameter(parameter)) {
                provided->getEquivalentPopupMenu().showMenuAsync(
                    juce::PopupMenu::Options().withTargetComponent(event.eventComponent)
                                              .withMousePosition());
                return;
            }
        }
        // Some hosts/versions don't provide native automation menu access.
        // Keep right-click useful with explicit, honest instructions.
        juce::PopupMenu fallback;
        fallback.addItem("Automatisation : mode d'emploi FL Studio...", [this] {
            juce::AlertWindow::showMessageBoxAsync(
                juce::AlertWindow::InfoIcon, "Automatisation FL Studio",
                "Tournez le potentiometre, puis dans FL Studio :\n"
                "Tools > Last tweaked > Create automation clip.\n\n"
                "Autre possibilite : Browser > Current project > Generators > Acid303 Solo.");
        });
        fallback.showMenuAsync(juce::PopupMenu::Options()
                                   .withTargetComponent(event.eventComponent)
                                   .withMousePosition());
    };
    c.slider->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    if (juce::String(id) == "masterVolume")
        c.slider->setTooltip("VOLUME GENERAL : unique potard de volume visible, niveau final apres les effets. Clic droit : automatisation FL Studio.");
    c.slider->setRotaryParameters(juce::MathConstants<float>::pi*1.25f,
                                   juce::MathConstants<float>::pi*2.75f,true);
    c.slider->setTextBoxStyle(juce::Slider::TextBoxBelow,false,68,18);
    c.slider->setColour(juce::Slider::textBoxTextColourId,neonGreen);
    c.slider->setColour(juce::Slider::textBoxBackgroundColourId,juce::Colour(0xff05080a));
    c.slider->setColour(juce::Slider::textBoxOutlineColourId,neonGreen);
    c.slider->setColour(juce::Slider::textBoxHighlightColourId,neonGreen);
    c.slider->setLookAndFeel(&knobLook);
    c.label=std::make_unique<juce::Label>();
    keepHostKeyboardFocus(*c.label);
    c.label->setText(title,juce::dontSendNotification);
    c.label->setFont(juce::Font(juce::FontOptions(11.0f).withStyle("Bold")));
    c.label->setColour(juce::Label::textColourId,neonGreen);
    c.label->setJustificationType(juce::Justification::centred);
    addAndMakeVisible(*c.slider);
    addAndMakeVisible(*c.label);
    c.attach=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.params,id,*c.slider);
    controls.emplace_back(std::move(c));
}
void Acid303SoloEditor::addEffectButton(const char* id) {
    EffectButton e;
    e.button=std::make_unique<juce::TextButton>("OFF");
    keepHostKeyboardFocus(*e.button);
    e.button->setClickingTogglesState(true);
    e.button->setColour(juce::TextButton::buttonColourId,juce::Colour(0xff7d0c1f));
    e.button->setColour(juce::TextButton::buttonOnColourId,juce::Colour(0xff0f3517));
    e.button->setColour(juce::TextButton::textColourOffId,juce::Colours::white);
    e.button->setColour(juce::TextButton::textColourOnId,neonGreen);
    e.button->setTooltip(juce::String("Bypass: ")+id+" - OFF = son sans cet effet");
    auto* b=e.button.get();
    b->onClick=[b] { b->setButtonText(b->getToggleState() ? "ON" : "OFF"); };
    addAndMakeVisible(*e.button);
    e.attach=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor.params,id,*e.button);
    effectButtons.emplace_back(std::move(e));
}
void Acid303SoloEditor::arrange(int index, int x, int y, int w, int h) {
    const float sx = float(getWidth()) / virtualWidth;
    const float sy = float(getHeight()) / virtualHeight;
    auto& c = controls[static_cast<size_t>(index)];
    const auto b = [sx,sy](float xx, float yy, float ww, float hh) {
        return juce::Rectangle<int>(juce::roundToInt(xx*sx),juce::roundToInt(yy*sy),
                                    juce::roundToInt(ww*sx),juce::roundToInt(hh*sy));
    };
    c.slider->setBounds(b(float(x),float(y),float(w),float(h-19)));
    c.label->setBounds(b(float(x-4),float(y+h-21),float(w+8),17.0f));
}
void Acid303SoloEditor::resized() {
    const float sx = float(getWidth()) / virtualWidth;
    const float sy = float(getHeight()) / virtualHeight;
    const auto B = [sx,sy](float x,float y,float w,float h) {
        return juce::Rectangle<int>(juce::roundToInt(x*sx),juce::roundToInt(y*sy),
                                    juce::roundToInt(w*sx),juce::roundToInt(h*sy));
    };
    tempoLabel.setBounds(B(715,18,246,30));
    skinLabel.setBounds(B(964,18,68,30));
    skinChoice.setBounds(B(1038,18,133,30));
    rateLabel.setBounds(B(309,608,76,28));
    rateChoice.setBounds(B(389,607,194,28));
    modeButton.setBounds(B(589,607,267,28));
    // Patch selector (left/right buttons) and file operations above OSC / FILTER / ENV / OUTPUT.
    previousPresetButton.setBounds(B(42,79,34,31));
    presetNameLabel.setBounds(B(86,79,355,31));
    nextPresetButton.setBounds(B(449,79,34,31));
    savePresetButton.setBounds(B(492,79,133,31));
    loadPresetButton.setBounds(B(634,79,133,31));
    presetFolderButton.setBounds(B(776,79,119,31));
    updateButton.setBounds(B(932,79,225,31));
    sequenceGrid.setBounds(B(38,639,1123,221));
    exportMidiButton.setBounds(B(810,862,100,25));
    importMidiButton.setBounds(B(914,862,100,25));
    midiFolderButton.setBounds(B(1018,862,143,25));
    // Leave breathing room below the TIME/FEEDBACK/WIDTH/DRY-WET labels
    // and keep the synced division entirely INSIDE the Delay panel (bottom y=589).
    delaySubdivision.setBounds(B(707,564,115,16));
    delayModeChoice.setBounds(B(646,436,136,28));

    // 303 monosynth: OSC / FILTER / ENVELOPE / MASTER.
    arrange(0,61,201,89,111); arrange(1,165,201,89,111); arrange(2,269,201,89,111);
    arrange(3,390,201,81,111); arrange(4,473,201,81,111);
    arrange(5,556,201,81,111); arrange(6,639,201,81,111);
    arrange(7,741,201,62,111); arrange(8,805,201,62,111);
    arrange(9,869,201,62,111); arrange(10,933,201,62,111);
    arrange(11,1032,200,105,114);

    // Compressor at LEFT of distortion. All five dials are automatable.
    arrange(22,40,472,58,92); arrange(23,102,472,58,92);
    arrange(24,164,472,58,92); arrange(25,226,472,58,92);
    arrange(26,288,472,58,92);

    // Distortion, synced delay, and stereo reverb follow the compressor.
    arrange(12,371,472,57,92); arrange(13,436,472,57,92);
    arrange(14,501,472,57,92); arrange(19,566,472,57,92);
    arrange(15,645,472,56,92); arrange(16,706,472,56,92);
    arrange(27,766,472,56,92); arrange(20,827,472,56,92);
    arrange(17,915,472,73,92); arrange(18,999,472,73,92);
    arrange(21,1083,472,73,92);

    prevDistType.setBounds(B(374,436,31,28));
    distTypeName.setBounds(B(409,436,114,28));
    nextDistType.setBounds(B(527,436,31,28));
    effectButtons[0].button->setBounds(B(237,436,101,28)); // Compressor
    effectButtons[1].button->setBounds(B(564,436,57,28));  // Distortion
    effectButtons[2].button->setBounds(B(793,436,88,28));  // Delay
    effectButtons[3].button->setBounds(B(1070,436,87,28)); // Reverb
}
void Acid303SoloEditor::paint(juce::Graphics& g) {
    const auto theme = resolveSkinThemeByName(getStoredSkinName(processor));
    const auto accent = theme.accent;
    const auto soft = theme.soft;
    g.fillAll(theme.deep);
    g.addTransform(juce::AffineTransform::scale(float(getWidth()) / virtualWidth,
                                                float(getHeight()) / virtualHeight));
    auto full = juce::Rectangle<float>(8.0f,8.0f,1184.0f,884.0f);
    g.setColour(theme.body);
    g.fillRoundedRectangle(full,8.0f);
    drawCrackTexture(g, full.reduced(4.0f), theme);
    g.setColour(accent.withAlpha(0.9f));
    g.drawRoundedRectangle(full.reduced(0.8f),8.0f,1.6f);

    g.setColour(accent);
    g.setFont(juce::Font(juce::FontOptions(30.0f).withStyle("Bold")));
    g.drawText("ACID 303",25,17,250,37,juce::Justification::centredLeft);


    g.setColour(juce::Colours::black.withAlpha(0.5f));
    g.fillRoundedRectangle(854.0f,15.0f,315.0f,38.0f,4.0f);

    panel(g,{23,68,1154,316},"","",false,theme);

    // Dedicated preset bar drawn above the synth section so every skin keeps a clean, uncut zone.
    const auto presetBar = juce::Rectangle<float>(29.0f, 74.0f, 872.0f, 42.0f);
    g.setColour(theme.deep.brighter(0.06f));
    g.fillRoundedRectangle(presetBar, 5.0f);
    g.setColour(accent.withAlpha(0.95f));
    g.drawRoundedRectangle(presetBar, 5.0f, 1.1f);
    g.setColour(accent.withAlpha(0.08f));
    g.fillRoundedRectangle(presetBar.reduced(1.5f), 4.0f);
    panel(g,{35,126,333,246},"OSCILLATOR","WAVE / SHAPE / TUNE",false,theme);
    panel(g,{377,126,347,246},"FILTER","CUTOFF / RESO / LFO",false,theme);
    panel(g,{733,126,264,246},"ENVELOPE","SLIDE / ACCENT",false,theme);
    panel(g,{1006,126,160,246},"OUTPUT","MASTER",false,theme);

    panel(g,{23,391,331,198},"COMPRESSOR","3-BAND",true,theme);
    panel(g,{362,391,267,198},"DISTORTION","TUBE / BITCRUSHER",true,theme);
    panel(g,{637,391,255,198},"DELAY","STEREO / SYNC BPM",true,theme);
    panel(g,{900,391,277,198},"REVERB","STEREO",true,theme);

    panel(g,{23,600,1154,288},"16-STEP SEQUENCER","",false,theme);
    g.setColour(soft);
    g.setFont(juce::Font(juce::FontOptions(12.5f).withStyle("Bold")));
    g.drawText("MIDI : velocite = accent | chevauchement = slide | import/export dans Documents/Acid303/midi", 
               47,866,752,16,juce::Justification::centredLeft);
}

void Acid303SoloEditor::applySkin(const juce::String& skinName) {
    const auto theme = resolveSkinThemeByName(skinName);
    const auto accent = theme.accent;
    const auto soft = theme.soft;
    const bool lightTheme = theme.panel.getPerceivedBrightness() > 0.65f;
    knobLook.setAccentColour(accent);
    for (auto& c : controls) {
        c.slider->setColour(juce::Slider::textBoxBackgroundColourId, theme.panel);
        c.slider->setColour(juce::Slider::textBoxTextColourId, accent);
        c.slider->setColour(juce::Slider::textBoxOutlineColourId, accent);
        c.slider->setColour(juce::Slider::textBoxHighlightColourId, accent);
        c.label->setColour(juce::Label::textColourId, accent);
        c.slider->repaint();
    }
    for (auto* label : {&presetNameLabel, &skinLabel, &rateLabel, &tempoLabel,
                        &distTypeName, &delaySubdivision})
        label->setColour(juce::Label::textColourId, accent);
    delaySubdivision.setColour(juce::Label::textColourId, soft);
    distTypeName.setColour(juce::Label::backgroundColourId, theme.panel);
    for (auto* choice : {&rateChoice, &skinChoice, &delayModeChoice}) {
        choice->setColour(juce::ComboBox::backgroundColourId, theme.panel);
        choice->setColour(juce::ComboBox::textColourId, accent);
        choice->setColour(juce::ComboBox::outlineColourId, accent);
        choice->setColour(juce::ComboBox::arrowColourId, accent);
    }
    for (auto* button : {&modeButton, &previousPresetButton, &nextPresetButton,
                         &savePresetButton, &loadPresetButton,
                         &presetFolderButton, &updateButton, &exportMidiButton, &importMidiButton,
                         &midiFolderButton, &prevDistType, &nextDistType}) {
        button->setColour(juce::TextButton::buttonColourId, theme.panel);
        button->setColour(juce::TextButton::textColourOffId, accent);
    }
    for (auto& effect : effectButtons) {
        effect.button->setColour(juce::TextButton::buttonColourId,
            lightTheme ? theme.panel.darker(0.09f) : theme.fxGlow);
        effect.button->setColour(juce::TextButton::buttonOnColourId,
            lightTheme ? theme.panel.darker(0.20f) : theme.panel.brighter(0.12f));
        effect.button->setColour(juce::TextButton::textColourOffId, accent);
        effect.button->setColour(juce::TextButton::textColourOnId, accent);
    }
    if (updateOffer.newer) updateButton.setColour(juce::TextButton::buttonColourId, theme.fxAccent.darker(0.55f));
    activeSkinName = theme.name;
    sequenceGrid.repaint();
    repaint();
}

void Acid303SoloEditor::timerCallback() {
    acid303update::Offer checked;
    if (updateWorker.takeResult(checked)) {
        updateOffer = checked;
        displayUpdateStatus();
    }
    if (++updateCheckTicks >= 8 * 3600) { updateCheckTicks = 0; checkForUpdates(); }
    // Look for newly copied or removed external .acid303skin files every 2s.
    // Do not interrupt the user while the dropdown menu is open.
    if (++skinScanTicks >= 16) {
        skinScanTicks = 0;
        if (!skinChoice.isPopupActive()) refreshSkinChoices();
    }
    const auto selectedSkin = getStoredSkinName(processor);
    if (activeSkinName != selectedSkin) applySkin(selectedSkin);
    if (availableSkinNames.indexOf(selectedSkin) >= 0 && skinChoice.getSelectedItemIndex() != availableSkinNames.indexOf(selectedSkin))
        skinChoice.setSelectedItemIndex(availableSkinNames.indexOf(selectedSkin), juce::dontSendNotification);
    const auto shown = "PRESET : " + processor.getCurrentPresetName();
    if (presetNameLabel.getText() != shown)
        presetNameLabel.setText(shown, juce::dontSendNotification);
    const float bpm=processor.hostBpm.load(std::memory_order_relaxed);
    const bool valid=processor.hostTempoAvailable.load(std::memory_order_relaxed);
    tempoLabel.setText((valid ? "FL STUDIO  " : "BPM FALLBACK  ")
                       + juce::String(bpm,1) + " BPM",juce::dontSendNotification);
    const float raw=processor.params.getRawParameterValue("delayTime")->load(std::memory_order_relaxed);
    const int i=juce::jlimit(0,8,static_cast<int>(raw*8.999f));
    const std::array<juce::String,9> divisions = {
        "1/16", "1/8", "3/16", "1/4", "3/8", "1/2", "3/4", "1/1", "2/1"
    };
    delaySubdivision.setText("SYNC: " + divisions[static_cast<size_t>(i)],juce::dontSendNotification);
    const std::array<juce::String,2> typeNames = { "TUBE", "BITCRUSHER" };
    const int selected = juce::jlimit(0,1,static_cast<int>(
        processor.params.getRawParameterValue("distType")->load(std::memory_order_relaxed) + 0.5f));
    distTypeName.setText(typeNames[static_cast<size_t>(selected)], juce::dontSendNotification);
    for (auto& effect : effectButtons)
        effect.button->setButtonText(effect.button->getToggleState() ? "ON" : "OFF");
    modeButton.setButtonText(processor.getPlayMode() == 0 ? "MODE : PIANO ROLL" : (processor.currentSequenceStep.load(std::memory_order_relaxed) < 0 ? "SEQUENCEUR : ATTENTE MIDI" : "SEQUENCEUR : ACTIF"));
    sequenceGrid.repaint();
}


void Acid303SoloEditor::ensureFactorySkinFiles() {
    if (auto result = Acid303SoloProcessor::prepareSkinFolder(); result.failed())
        return;
    const auto folder = Acid303SoloProcessor::getUserSkinFolder();
    for (const auto& theme : factoryThemes()) {
        const auto file = folder.getChildFile(skinFileName(theme.name));
        if (!file.existsAsFile())
            writeSkinFile(theme, file);
    }
}

void Acid303SoloEditor::refreshSkinChoices() {
    std::set<juce::String> uniqueNames;
    for (const auto& theme : factoryThemes())
        uniqueNames.insert(theme.name);
    if (Acid303SoloProcessor::prepareSkinFolder().wasOk()) {
        const auto folder = Acid303SoloProcessor::getUserSkinFolder();
        for (const auto& file : folder.findChildFiles(juce::File::findFiles, false, "*.acid303skin")) {
            SkinTheme loaded;
            if (loadSkinFile(file, loaded) && loaded.name.isNotEmpty())
                uniqueNames.insert(loaded.name);
        }
    }
    juce::StringArray newlyAvailable;
    for (const auto& name : uniqueNames)
        newlyAvailable.add(name);
    // Avoid closing a currently open popup or resetting the selected entry.
    if (newlyAvailable.joinIntoString("\n") == availableSkinNames.joinIntoString("\n"))
        return;
    availableSkinNames = newlyAvailable;
    skinChoice.clear(juce::dontSendNotification);
    for (int i = 0; i < availableSkinNames.size(); ++i)
        skinChoice.addItem(availableSkinNames[i], i + 1);
    const auto wanted = getStoredSkinName(processor);
    const int idx = availableSkinNames.indexOf(wanted);
    skinChoice.setSelectedItemIndex(idx >= 0 ? idx : 0, juce::dontSendNotification);
}

void Acid303SoloEditor::checkForUpdates() {
    if (!updateWorker.isThreadRunning())
        updateWorker.startThread();
}

void Acid303SoloEditor::displayUpdateStatus() {
    if (!updateOffer.configured) {
        updateButton.setButtonText("MAJ : NON CONFIGUREE");
        updateButton.setTooltip("L'editeur doit configurer l'URL publique du fichier latest.json.");
    } else if (!updateOffer.valid) {
        updateButton.setButtonText("MAJ : HORS LIGNE");
        updateButton.setTooltip(updateOffer.error + "\nCliquez pour relancer la verification.");
    } else if (updateOffer.newer) {
        updateButton.setButtonText("MAJ : v" + updateOffer.version + " DISPONIBLE");
        updateButton.setTooltip(updateOffer.notes + "\nCliquez pour ouvrir la page des Releases GitHub.");
    } else {
        updateButton.setButtonText("A JOUR : v" + acid303update::installedVersion());
        updateButton.setTooltip("Version installee a jour. Cliquez pour rechercher une nouvelle version.");
    }
    updateButton.setColour(juce::TextButton::buttonColourId,
                           updateOffer.newer ? juce::Colour(0xff5d2512) : juce::Colour(0xff06110a));
}

void Acid303SoloEditor::handleUpdateClick() {
    if (updateOffer.valid && updateOffer.newer) {
        juce::URL(updateOffer.downloadUrl).launchInDefaultBrowser();
        return;
    }
    updateButton.setButtonText("MAJ : VERIFICATION...");
    checkForUpdates();
}

void Acid303SoloEditor::chooseMidiFile(bool saving) {
    if (auto result = Acid303SoloProcessor::prepareMidiFolder(); result.failed()) {
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
            "Dossier MIDI", result.getErrorMessage());
        return;
    }
    const auto folder = Acid303SoloProcessor::getUserMidiFolder();
    const auto suggested = saving ? folder.getChildFile("Ma sequence.mid") : folder;
    midiChooser = std::make_unique<juce::FileChooser>(
        saving ? "Exporter une sequence MIDI Acid303" : "Importer une sequence MIDI",
        suggested, saving ? "*.mid" : "*.mid;*.midi", true);
    const int midiFileFlags = juce::FileBrowserComponent::canSelectFiles
        | (saving ? juce::FileBrowserComponent::saveMode
                  | juce::FileBrowserComponent::warnAboutOverwriting
                  : juce::FileBrowserComponent::openMode);
    const juce::Component::SafePointer<Acid303SoloEditor> safe(this);
    midiChooser->launchAsync(midiFileFlags, [safe,saving](const juce::FileChooser& chooser) {
        if (safe == nullptr) return;
        auto chosen = chooser.getResult();
        if (chosen == juce::File()) return;
        const auto result = saving ? safe->processor.exportMidiToFile(chosen)
                                   : safe->processor.importMidiFromFile(chosen);
        if (result.failed()) {
            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                "MIDI Acid303", result.getErrorMessage());
        } else if (!saving) {
            safe->sequenceGrid.repaint();
            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon,
                "MIDI importe", "Les 16 pas du sequenceur ont ete actualises.\n"
                "Les fichiers MIDI externes peuvent perdre les informations propres a Acid303.");
        }
    });
}

// A host-visible choice parameter, not a UI-only switch. This is saved with
// FL Studio projects and can also be automated from FL Studio.
void Acid303SoloEditor::stepDistType(int delta) {
    auto* parameter = processor.params.getParameter("distType");
    if (parameter == nullptr) return;
    const int current = juce::jlimit(0, 1, static_cast<int>(
        processor.params.getRawParameterValue("distType")->load(std::memory_order_relaxed) + 0.5f));
    const int next = (current + delta + 2) % 2;
    parameter->beginChangeGesture();
    parameter->setValueNotifyingHost(parameter->convertTo0to1(static_cast<float>(next)));
    parameter->endChangeGesture();
    timerCallback();
}


void Acid303SoloEditor::stepUserPreset(int direction) {
    const auto result = Acid303SoloProcessor::preparePresetFolder();
    if (result.failed()) {
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
            "Presets", result.getErrorMessage());
        return;
    }
    const auto discovered = Acid303SoloProcessor::getUserPresetFolder().findChildFiles(
        juce::File::findFiles, false, "*.acid303preset");
    std::vector<juce::File> files;
    for (const auto& file : discovered) files.push_back(file);
    std::sort(files.begin(), files.end(), [](const juce::File& a, const juce::File& b) {
        return a.getFileNameWithoutExtension().compareIgnoreCase(b.getFileNameWithoutExtension()) < 0;
    });
    if (files.empty()) {
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon, "Presets",
            "Aucun preset dans Documents/Acid303/presets.\n"
            "Enregistrez d'abord un son avec SAVE PRESET.");
        return;
    }
    const auto current = processor.getCurrentPresetName();
    int currentIndex = -1;
    for (int i = 0; i < static_cast<int>(files.size()); ++i) {
        if (files[static_cast<size_t>(i)].getFileNameWithoutExtension().equalsIgnoreCase(current)) {
            currentIndex = i;
            break;
        }
    }
    const int n = static_cast<int>(files.size());
    const int chosen = currentIndex < 0 ? (direction > 0 ? 0 : n - 1)
                                        : (currentIndex + (direction > 0 ? 1 : -1) + n) % n;
    const auto chosenFile = files[static_cast<size_t>(chosen)];
    if (auto loadResult = processor.loadPresetFromFile(chosenFile); loadResult.failed()) {
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
            "Preset Acid303", loadResult.getErrorMessage());
    } else {
        timerCallback();
        repaint();
    }
}

void Acid303SoloEditor::choosePresetFile(bool saving) {
    auto folder = Acid303SoloProcessor::getDefaultPresetFolder();
    if (auto result = Acid303SoloProcessor::preparePresetFolder(); result.failed()) {
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
            "Dossier des presets", result.getErrorMessage());
        return;
    }
    // When saving edits to a loaded preset, propose its current name rather
    // than the generic "Mon preset" filename. The native Save dialog remains
    // editable, and warnAboutOverwriting requests confirmation before replacing
    // an existing preset. The user can also type a new name to keep both.
    juce::File suggested = folder;
    if (saving) {
        auto currentName = processor.getCurrentPresetName().trim();
        if (currentName.isEmpty() || currentName.equalsIgnoreCase("INIT PATCH"))
            currentName = "Mon preset";
        const auto legalName = juce::File::createLegalFileName(currentName);
        suggested = folder.getChildFile((legalName.isNotEmpty() ? legalName : juce::String("Mon preset"))
                                         + ".acid303preset");
    }
    presetChooser = std::make_unique<juce::FileChooser>(
        saving ? "Enregistrer un preset Acid303" : "Charger un preset Acid303",
        suggested, "*.acid303preset", true);
    const auto presetFileFlags = saving
        ? (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles | juce::FileBrowserComponent::warnAboutOverwriting)
        : (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles);
    juce::Component::SafePointer<Acid303SoloEditor> safe(this);
    presetChooser->launchAsync(presetFileFlags, [safe,saving](const juce::FileChooser& chooser) {
        if (safe == nullptr) return;
        const auto selected = chooser.getResult();
        if (selected == juce::File()) return;
        const auto result = saving
            ? safe->processor.savePresetToFile(selected)
            : safe->processor.loadPresetFromFile(selected);
        if (result.failed())
            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                "Preset Acid303", result.getErrorMessage());
        else if (saving) {
            const auto savedFile = selected.hasFileExtension(".acid303preset")
                ? selected : selected.withFileExtension(".acid303preset");
            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon,
                "Preset enregistre", "Preset sauvegarde dans :\n" + savedFile.getFullPathName());
        } else {
            safe->timerCallback();
        }
    });
}

namespace {
juce::String gridNoteName(int note) {
    static const std::array<juce::String,12> names = {
        "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
    };
    note = juce::jlimit(0, 127, note);
    return names[static_cast<size_t>(note % 12)] + juce::String(note / 12 - 1);
}
}

void Acid303SequenceGrid::paint(juce::Graphics& g) {
    const auto theme = resolveSkinThemeByName(getStoredSkinName(processor));
    const auto accent = theme.accent;
    const auto soft = theme.soft;
    const bool lightTheme = theme.panel.getPerceivedBrightness() > 0.65f;
    const float scaleX = float(getWidth()) / 1123.0f;
    const float scaleY = float(getHeight()) / 221.0f;
    g.addTransform(juce::AffineTransform::scale(scaleX,scaleY));
    const auto bounds = juce::Rectangle<int>(0,0,1123,221);
    g.setColour(lightTheme ? theme.panel : juce::Colour(0xff101519));
    g.fillRoundedRectangle(bounds.toFloat(), 4.0f);
    const float cell = float(1123 - labelWidth) / 16.0f;
    const int playing = processor.getPlayMode() == 1
        ? processor.currentSequenceStep.load(std::memory_order_relaxed) : -1;
    // Display order is reversed compared with the original grid.
    // Model data and audio behavior are unchanged.
    const std::array<juce::String,4> rows = {"ACCENT", "SLIDE", "NOTE", "ON / OFF"};

    // Step numbers and a selectable LED per column choose the LOOP LENGTH.
    // The first pad at the left is step 1, and numbering grows to step 16
    // at the right, using exactly the same column boundaries as the pads.
    g.setColour(lightTheme ? theme.body : juce::Colour(0xff151c20));
    g.fillRoundedRectangle(juce::Rectangle<float>(0.0f, 0.0f,
                           1123.0f, float(headerHeight)), 3.0f);
    g.setColour(accent);
    g.setFont(juce::Font(juce::FontOptions(14.0f).withStyle("Bold")));
    g.drawText("BOUCLE", 10, 1, labelWidth-20, 19, juce::Justification::centredLeft);
    g.setColour(soft);
    g.drawText(juce::String(processor.getSequenceLength()) + " / 16", 10, 19, labelWidth-20, 20,
               juce::Justification::centredLeft);
    const int loopLength = processor.getSequenceLength();
    for (int col=0; col<16; ++col) {
        const int left = labelWidth + juce::roundToInt(col * cell);
        const int right = labelWidth + juce::roundToInt((col + 1) * cell);
        const auto hdr = juce::Rectangle<int>(left, 0, right-left, headerHeight);
        if (col == playing) {
            g.setColour(accent.withAlpha(0.20f));
            g.fillRect(hdr.reduced(2, 2));
            g.setColour(accent.withAlpha(0.10f));
            g.fillRect(left, headerHeight, right-left, rowHeight*4);
        }
        // Bars every four beats give a clear left-to-right reading.
        if (col % 4 == 0) {
            g.setColour(accent.withAlpha(0.35f));
            g.fillRect(left, 2, 1, headerHeight-4);
        }
        g.setColour(col < loopLength ? accent : theme.dim);
        g.setFont(juce::Font(juce::FontOptions(13.0f).withStyle("Bold")));
        g.drawText(juce::String(col + 1), juce::Rectangle<int>(left, 0, right-left, 19), juce::Justification::centred);
        const float ledX = 0.5f * float(left + right) - 7.0f;
        g.setColour(juce::Colour(0xff010302));
        g.fillEllipse(ledX - 1.5f, 24.0f, 17.0f, 17.0f);
        // Orange: selected loop endpoint, green: within loop, grey: excluded.
        g.setColour(col + 1 == loopLength ? juce::Colour(0xfffb355a) :
                    col + 1 < loopLength ? accent : juce::Colour(0xff233128));
        g.fillEllipse(ledX, 25.5f, 14.0f, 14.0f);
    }
    for (int row=0; row<4; ++row) {
        const int top=headerHeight+row*rowHeight;
        g.setColour(lightTheme ? (row%2 == 0 ? theme.panel : theme.body)
                                   : (row%2 == 0 ? juce::Colour(0xff06100a) : juce::Colour(0xff040b07)));
        g.fillRect(0,top,labelWidth-1,rowHeight-2);
        g.setColour(accent);
        g.drawText(rows[static_cast<size_t>(row)], 10,top, labelWidth-20,rowHeight-1,
                   juce::Justification::centredLeft);
        for (int col=0; col<16; ++col) {
            const auto step=processor.getStep(col);
            const int left=labelWidth+juce::roundToInt(col*cell);
            const int right=labelWidth+juce::roundToInt((col+1)*cell);
            juce::Rectangle<float> pad(float(left+4),float(top+6),float(right-left-8),float(rowHeight-12));
            const bool isOn = row==0 ? step.accent : row==1 ? step.slide : row==3 ? step.on : true;
            const auto onColour = row==0 ? juce::Colour(0xff8f1c33) : row==1 ? juce::Colour(0xff0c4826)
                                       : row==2 ? juce::Colour(0xff0a0f0d) : juce::Colour(0xff0f3517);
            g.setColour(lightTheme ? (isOn ? theme.panel.darker(0.11f) : theme.panel)
                                   : (col < loopLength ? (isOn ? onColour : juce::Colour(0xff0d1510))
                                                       : juce::Colour(0xff0a0e0b)));
            g.fillRoundedRectangle(pad,4.0f);
            if (row == 2) {
                // The pitch is ALWAYS white on a very dark pad: never yellow text
                // on amber; label stays readable even at the minimum window size.
                g.setColour(accent);
                g.drawRoundedRectangle(pad.reduced(0.8f),3.0f,1.6f);
                g.setColour(col < loopLength ? soft : theme.dim.brighter(0.15f));
                g.setFont(juce::Font(juce::FontOptions(16.0f).withStyle("Bold")));
                g.drawText(gridNoteName(step.note), pad.toNearestInt(), juce::Justification::centred);
            } else if (isOn) {
                g.setColour(soft);
                g.setFont(juce::Font(juce::FontOptions(13.0f).withStyle("Bold")));
                g.drawText(row==0 ? "A" : row==1 ? "S" : "ON", pad.toNearestInt(), juce::Justification::centred);
            } else {
                g.setColour(theme.dim.brighter(0.05f));
                g.setFont(juce::Font(juce::FontOptions(12.0f)));
                g.drawText("-", pad.toNearestInt(), juce::Justification::centred);
            }
        }
    }
}

void Acid303SequenceGrid::mouseDown(const juce::MouseEvent& e) {
    const float x = float(e.x) * (1123.0f / float(getWidth()));
    const float y = float(e.y) * (221.0f / float(getHeight()));
    const float cell = float(1123 - labelWidth) / 16.0f;
    if (x < labelWidth) return;
    const int col=juce::jlimit(0,15,static_cast<int>((x-labelWidth)/cell));
    if (y < headerHeight) {
        processor.setSequenceLength(col + 1); // Click LED / number
        repaint();
        return;
    }
    const int row=static_cast<int>((y-headerHeight)/rowHeight);
    if (row<0 || row>3) return;
    auto step=processor.getStep(col);
    if (row==0) step.accent=!step.accent;
    else if (row==1) step.slide=!step.slide;
    else if (row==2) { showNoteMenu(col); return; }
    else step.on=!step.on;
    processor.setStep(col,step);
    repaint();
}

void Acid303SequenceGrid::mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) {
    // Wheel control applies only while the mouse is actually over a NOTE pad,
    // not the row label or the gaps between pads. Other rows are unaffected.
    constexpr int noteRow = 2;
    const int padTop = headerHeight + noteRow * rowHeight + 6;
    const int padBottom = headerHeight + (noteRow + 1) * rowHeight - 6;
    const float x = float(e.x) * (1123.0f / float(getWidth()));
    const float y = float(e.y) * (221.0f / float(getHeight()));
    if (x < labelWidth || x >= 1123.0f || y < padTop ||
        y >= padBottom || wheel.deltaY == 0.0f) {
        juce::Component::mouseWheelMove(e, wheel);
        return;
    }
    const float cell = float(1123 - labelWidth) / 16.0f;
    const int col = juce::jlimit(0, 15, static_cast<int>((x - labelWidth) / cell));
    const int left = labelWidth + juce::roundToInt(col * cell);
    const int right = labelWidth + juce::roundToInt((col + 1) * cell);
    // Match the exact horizontal bounds drawn in paint().
    if (x < left + 4 || x >= right - 4) {
        juce::Component::mouseWheelMove(e, wheel);
        return;
    }
    auto step = processor.getStep(col);
    const int semitone = wheel.deltaY > 0.0f ? 1 : -1;
    step.note = static_cast<uint8_t>(juce::jlimit(24, 95, static_cast<int>(step.note) + semitone));
    processor.setStep(col, step);
    repaint();
}

void Acid303SequenceGrid::showNoteMenu(int col) {
    juce::PopupMenu menu;
    const int selected=processor.getStep(col).note;
    for (int octave=1; octave<=6; ++octave) {
        juce::PopupMenu sub;
        for (int half=0;half<12;++half) {
            const int note=(octave+1)*12+half;
            sub.addItem(note+1,gridNoteName(note),true,note==selected);
        }
        menu.addSubMenu("Octave " + juce::String(octave),sub);
    }
    const juce::Component::SafePointer<Acid303SequenceGrid> safe(this);
    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(this),
                       [safe,col](int choice) {
        if (safe != nullptr && choice>0) {
            auto s=safe->processor.getStep(col);
            s.note=static_cast<uint8_t>(juce::jlimit(0,127,choice-1));
            safe->processor.setStep(col,s);
            safe->repaint();
        }
    });
}
