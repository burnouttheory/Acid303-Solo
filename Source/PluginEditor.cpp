#include "PluginEditor.h"
#include "BinaryData.h"
#include <array>
#include <algorithm>
#include <cmath>
#include <set>

namespace {
using acid303i18n::tr;

// User-wide selection (shared across FL Studio sessions). Never stored inside
// Factory/User and never bundled with the plugin: updates preserve this file.
juce::File getPresetDirectoryHistoryFile() {
    return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
        .getChildFile("Acid303Solo").getChildFile("last_preset_folder.txt");
}

juce::File getLastPresetDirectory() {
    const auto root = Acid303SoloProcessor::getUserPresetFolder();
    const auto history = getPresetDirectoryHistoryFile();
    if (history.existsAsFile()) {
        const auto remembered = history.loadFileAsString().trim();
        if (remembered.isNotEmpty() && juce::File::isAbsolutePath(remembered)) {
            const juce::File candidate(remembered);
            if (candidate.isDirectory()) return candidate;
        }
    }
    // A deleted or moved folder must never prevent the native picker opening.
    return root;
}

void rememberLastPresetDirectory(const juce::File& directory) {
    if (!directory.isDirectory()) return;
    const auto history = getPresetDirectoryHistoryFile();
    if (history.getParentDirectory().createDirectory().failed()) return;
    juce::TemporaryFile temp(history);
    if (temp.getFile().replaceWithText(directory.getFullPathName()))
        (void) temp.overwriteTargetFileWithTemporary();
}

constexpr float virtualWidth = 1440.0f;
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

SkinTheme makeThemeV3Gray() {
    SkinTheme t; t.name="TB303 GREY V3";
    t.accent=juce::Colour(0xff64ee9b); t.soft=juce::Colour(0xffadffd1);
    t.body=juce::Colour(0xffc3c4c5); t.deep=juce::Colour(0xffacadb0);
    t.panel=juce::Colour(0xff202224); t.crackA=juce::Colour(0xffc3c5c7);
    t.crackB=juce::Colour(0xffa7aaac); t.dim=juce::Colour(0xff548d71);
    t.fxAccent=juce::Colour(0xff64ee9b); t.fxGlow=juce::Colour(0xff2a624c);
    return t;
}
juce::String defaultSkinName() { return "TB303 GREY V3"; }

// "V1 ORIGINAL" matches the 22 Oct 2026 reference screenshot palette.
// The associated geometry and custom pad/knob rendering are in this editor.
SkinTheme makeThemeV1() {
    SkinTheme t; t.name = "V1 ORIGINAL";
    t.accent = juce::Colour(0xff75e9ad); t.soft = juce::Colour(0xffe9f2ef);
    t.body = juce::Colour(0xff10181c); t.deep = juce::Colour(0xff0b1013);
    t.panel = juce::Colour(0xff151e23); t.crackA = t.panel; t.crackB = t.body;
    t.fxAccent = juce::Colour(0xffed6776); t.fxGlow = juce::Colour(0xff65232e); t.dim = juce::Colour(0xff586b72);
    return t;
}

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
    return { makeThemeV3Gray(), makeThemeV1(), makeThemeV25(), makeThemeV26(), makeThemeV24(), makeThemeFire() };
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
    // V3 Grey skin hot-replacement: the user's .acid303skin has priority over
    // the built-in version. Other factory skins keep their original behaviour.
    if (requestedName == "TB303 GREY V3") {
        const auto custom = Acid303SoloProcessor::getUserSkinFolder()
            .getChildFile("TB303_GREY_V3.acid303skin");
        SkinTheme replacement;
        if (loadSkinFile(custom, replacement) && replacement.name == requestedName) {
            // Legacy TB303 GREY was bundled in fluorescent yellow. If that
            // untouched old palette is still present, prefer the new green
            // factory palette without overwriting user-authored skin files.
            if (replacement.accent == juce::Colour(0xffffff08)
                && replacement.soft == juce::Colour(0xffffff75))
                return makeThemeV3Gray();
            return replacement;
        }
    }
    for (const auto& theme : factoryThemes()) if (theme.name == requestedName) return theme;
    const auto folder = Acid303SoloProcessor::getUserSkinFolder();
    const auto exact = folder.getChildFile(skinFileName(requestedName));
    SkinTheme t;
    if (loadSkinFile(exact, t)) return t;
    for (const auto& f : folder.findChildFiles(juce::File::findFiles, false, "*.acid303skin")) {
        SkinTheme candidate;
        if (loadSkinFile(f, candidate) && candidate.name == requestedName) return candidate;
    }
    return makeThemeV3Gray();
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

// V3 Grey Metal Yellow skin: non-interactive painting only. Everything here
// executes on JUCE's UI thread; no audio/host parameter is changed.
void paintBrushedSteel(juce::Graphics& g, juce::Rectangle<float> r) {
    g.saveState();
    g.reduceClipRegion(r.toNearestInt());
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xffdadbdd),r.getX(),r.getY(),
                      juce::Colour(0xff9b9da0),r.getRight(),r.getBottom(),false));
    g.fillRect(r);
    // Deterministic fine horizontal streaks: no random CPU work and no assets.
    for (int i = 0; i < 30; ++i) {
        const float yy = r.getY() + r.getHeight()*float(i+1)/31.0f;
        const float xx = r.getX() + float((i*37)%19);
        g.setColour(i%3 == 0 ? juce::Colours::white.withAlpha(0.070f)
                              : juce::Colours::black.withAlpha(0.036f));
        g.drawLine(xx, yy, r.getRight()-float((i*11)%16), yy, 0.65f);
    }
    g.restoreState();
}

void paintSteelScrew(juce::Graphics& g, float cx, float cy) {
    constexpr float r = 5.4f;
    g.setColour(juce::Colour(0xff3a3a3b));
    g.fillEllipse(cx-r-0.6f, cy-r-0.6f, 2*r+1.2f, 2*r+1.2f);
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xfffafafa),cx-r,cy-r,
                      juce::Colour(0xff717376),cx+r,cy+r,false));
    g.fillEllipse(cx-r+0.5f, cy-r+0.5f, 2*r-1.f, 2*r-1.f);
    g.setColour(juce::Colour(0xff292b2e));
    g.drawLine(cx-2.5f, cy-2.f, cx+2.5f, cy+2.f, 1.5f);
    g.setColour(juce::Colours::white.withAlpha(0.36f));
    g.drawLine(cx-2.5f, cy-3.f, cx+2.5f, cy+1.f, 0.6f);
}

void paintPanelScrews(juce::Graphics& g, juce::Rectangle<float> r) {
    constexpr float d=9.f;
    paintSteelScrew(g,r.getX()+d,r.getY()+d);
    paintSteelScrew(g,r.getRight()-d,r.getY()+d);
    paintSteelScrew(g,r.getX()+d,r.getBottom()-d);
    paintSteelScrew(g,r.getRight()-d,r.getBottom()-d);
}

void panel(juce::Graphics& g, juce::Rectangle<float> r, const juce::String& title,
           const juce::String& sub, bool redMode, const SkinTheme& theme) {
    if (theme.name == "TB303 GREY V3") {
        // User-approved metal look: silver interior, uniform soft neon green
        // hairline borders, dark title strips and screws at four panel corners.
        g.setColour(juce::Colours::black.withAlpha(0.23f));
        g.fillRoundedRectangle(r.translated(1.2f, 2.2f), 6.0f);
        g.saveState();
        g.reduceClipRegion(r.toNearestInt());
        paintBrushedSteel(g,r);
        g.restoreState();
        g.setColour(theme.accent);
        g.drawRoundedRectangle(r.reduced(0.7f),6.0f,1.15f);
        const auto header=juce::Rectangle<float>(r.getX()+9.0f,r.getY()+7.0f,
                                                 r.getWidth()-18.0f,29.0f);
        g.setColour(theme.panel);
        g.fillRoundedRectangle(header,3.5f);
        g.setColour(theme.accent);
        g.fillEllipse(header.getX()+8.0f,header.getY()+9.0f,10.0f,10.0f);
        g.setFont(juce::Font(juce::FontOptions(16.0f).withStyle("Bold")));
        g.drawText(title,header.withTrimmedLeft(26.0f).reduced(0,2),
                   juce::Justification::centredLeft);
        g.setFont(juce::Font(juce::FontOptions(10.0f).withStyle("Bold")));
        g.drawText(sub,header.reduced(14,2),juce::Justification::centredRight);
        paintPanelScrews(g,r);
        return;
    }
    if (theme.name == "V1 ORIGINAL") {
        // Reference: flat slate background, thin steel outlines and muted red FX sections.
        const auto outline = redMode ? juce::Colour(0xffad4352) : juce::Colour(0xff52636c);
        const auto inside = redMode ? juce::Colour(0xff24171b) : juce::Colour(0xff141c21);
        const auto header = redMode ? juce::Colour(0xff6b2932) : juce::Colour(0xff28363d);
        g.setColour(inside);
        g.fillRoundedRectangle(r, 6.0f);
        g.setColour(outline);
        g.drawRoundedRectangle(r.reduced(0.6f), 5.6f, 1.65f);
        const auto bar = juce::Rectangle<float>(r.getX()+8.0f,r.getY()+8.0f,r.getWidth()-16.0f,29.0f);
        g.setColour(header);
        g.fillRoundedRectangle(bar, 3.0f);
        g.setColour(redMode ? juce::Colour(0xfff06e80) : juce::Colour(0xff76e7ad));
        g.fillEllipse(bar.getX()+7.0f,bar.getY()+8.0f,11.0f,11.0f);
        g.setColour(juce::Colours::white);
        g.setFont(juce::Font(juce::FontOptions(15.0f).withStyle("Bold")));
        g.drawText(title,bar.withTrimmedLeft(26).reduced(0,2),juce::Justification::centredLeft);
        g.setColour(juce::Colour(0xffd2dcdf));
        g.setFont(juce::Font(juce::FontOptions(9.5f).withStyle("Bold")));
        g.drawText(sub,bar.reduced(14,2),juce::Justification::centredRight);
        return;
    }
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


// V2.0.3: MIDI configuration uses the FL Studio host's verified native
// Link to Controller / Multilink workflow. No experimental direct CC mapping.
// Host-side automation links are saved by FL Studio in its .flp project.
class Acid303MidiSetupPanel final : public juce::Component, private juce::Timer {
public:
    Acid303MidiSetupPanel() {
        setSize(690, 440);
        title.setFont(juce::Font(juce::FontOptions(19.0f).withStyle("Bold")));
        title.setColour(juce::Label::textColourId, juce::Colour(0xff90ffba));
        caption.setFont(juce::Font(juce::FontOptions(13.0f)));
        caption.setColour(juce::Label::textColourId, juce::Colour(0xffc6d5cf));
        feedback.setFont(juce::Font(juce::FontOptions(12.0f)));
        feedback.setColour(juce::Label::textColourId, juce::Colour(0xffa5ffc8));
        for (auto* label : {&title, &caption, &feedback}) addAndMakeVisible(*label);
        for (auto* button : {&openManual, &copySteps}) {
            button->setColour(juce::TextButton::buttonColourId, juce::Colour(0xff286749));
            button->setColour(juce::TextButton::textColourOffId, juce::Colours::white);
            keepHostKeyboardFocus(*button);
            addAndMakeVisible(*button);
        }
        openManual.onClick = [] {
            juce::URL("https://www.image-line.com/fl-studio-learning/fl-studio-online-manual/html/automation_linking.htm")
                .launchInDefaultBrowser();
        };
        copySteps.onClick = [this] {
            juce::SystemClipboard::copyTextToClipboard(getFLSteps());
            feedback.setText(tr("Instructions copiées dans le presse-papiers.",
                                "Instructions copied to clipboard."), juce::dontSendNotification);
        };
        applyPanelLanguage();
        startTimerHz(2);
    }

    void paint(juce::Graphics& g) override {
        g.fillAll(juce::Colour(0xff101b20));
        g.setColour(juce::Colour(0xff4d9f70));
        g.drawRect(getLocalBounds().reduced(1), 1);
        auto card = [&g](juce::Rectangle<int> area) {
            g.setColour(juce::Colour(0xff1a292d));
            g.fillRoundedRectangle(area.toFloat(), 6.0f);
            g.setColour(juce::Colour(0xff3d6b5b));
            g.drawRoundedRectangle(area.toFloat(), 6.0f, 1.0f);
        };
        card({19, 93, 652, 123});
        card({19, 224, 652, 117});
        g.setColour(juce::Colour(0xff98ffc5));
        g.setFont(juce::Font(juce::FontOptions(14.5f).withStyle("Bold")));
        g.drawText(tr("1. UN POTENTIOMÈTRE (testé avec AKAI MPK Mini)",
                      "1. ONE KNOB (tested with AKAI MPK Mini)"),
                   30, 105, 625, 22, juce::Justification::centredLeft);
        g.drawText(tr("2. PLUSIEURS POTENTIOMÈTRES (Multilink)",
                      "2. MULTIPLE KNOBS (Multilink)"),
                   30, 236, 625, 22, juce::Justification::centredLeft);
        g.setColour(juce::Colour(0xffe1eae7));
        g.setFont(juce::Font(juce::FontOptions(13.2f)));
        g.drawFittedText(tr("Activez votre contrôleur dans Options > MIDI Settings.\n"
                            "Clic droit sur CUTOFF (ou un autre potentiomètre) > Link to controller.\n"
                            "Tournez la commande MIDI physique, puis validez la fenêtre de liaison.",
                            "Enable your controller in Options > MIDI Settings.\n"
                            "Right-click CUTOFF (or another knob) > Link to controller.\n"
                            "Turn the hardware MIDI control, then confirm the link dialog."),
                         juce::Rectangle<int>(31, 134, 625, 74), juce::Justification::topLeft, 4);
        g.drawFittedText(tr("Activez Multilink dans la barre d'outils FL Studio.\n"
                            "Manipulez les paramètres Acid303 dans l'ordre souhaité, puis\n"
                            "tournez les potentiomètres physiques dans le même ordre.",
                            "Enable Multilink from the FL Studio toolbar.\n"
                            "Move Acid303 parameters in the desired order, then turn\n"
                            "the corresponding hardware knobs in the same order."),
                         juce::Rectangle<int>(31, 265, 625, 68), juce::Justification::topLeft, 4);
        g.setColour(juce::Colour(0xffc4d2ce));
        g.setFont(juce::Font(juce::FontOptions(12.0f)));
        g.drawFittedText(tr("Les affectations sont gérées par FL Studio : enregistrez le projet .flp.",
                            "Mappings are managed by FL Studio: save your .flp project."),
                         juce::Rectangle<int>(20, 345, 652, 22),
                         juce::Justification::centredLeft, 2);
    }

    void resized() override {
        title.setBounds(20, 13, 650, 30);
        caption.setBounds(20, 47, 650, 23);
        openManual.setBounds(19, 380, 315, 35);
        copySteps.setBounds(345, 380, 326, 35);
        feedback.setBounds(20, 420, 640, 16);
    }

private:
    juce::Label title, caption, feedback;
    juce::TextButton openManual, copySteps;
    int panelLanguageId = 0;

    juce::String getFLSteps() const {
        return tr("ACID303 SOLO - LIAISON MIDI FL STUDIO\n"
                  "1. Options > MIDI Settings : activer votre contrôleur MIDI.\n"
                  "2. Clic droit sur CUTOFF > Link to controller.\n"
                  "3. Tourner le potentiomètre physique puis valider.\n"
                  "Plusieurs paramètres : activer Multilink, manipuler les commandes "
                  "Acid303 puis tourner les potentiomètres physiques dans le même ordre.\n"
                  "Enregistrer le projet .flp.\n"
                  "https://www.image-line.com/fl-studio-learning/fl-studio-online-manual/html/automation_linking.htm",
                  "ACID303 SOLO - FL STUDIO MIDI LINK\n"
                  "1. Options > MIDI Settings: enable your MIDI controller.\n"
                  "2. Right-click CUTOFF > Link to controller.\n"
                  "3. Turn a physical knob and confirm.\n"
                  "For multiple parameters: enable Multilink, move the Acid303 controls "
                  "then turn your hardware knobs in the same order.\n"
                  "Save your .flp project.\n"
                  "https://www.image-line.com/fl-studio-learning/fl-studio-online-manual/html/automation_linking.htm");
    }

    void applyPanelLanguage() {
        panelLanguageId = static_cast<int>(acid303i18n::current());
        title.setText(tr("CONFIGURATION MIDI / ACID303 SOLO V3.0.0",
                          "MIDI SETUP / ACID303 SOLO V3.0.0"), juce::dontSendNotification);
        caption.setText(tr("Liaison MIDI via FL Studio — compatible avec les contrôleurs standard.",
                            "MIDI linking via FL Studio — compatible with standard controllers."),
                        juce::dontSendNotification);
        openManual.setButtonText(tr("OUVRIR LE MANUEL FL STUDIO", "OPEN FL STUDIO MANUAL"));
        copySteps.setButtonText(tr("COPIER LES INSTRUCTIONS", "COPY INSTRUCTIONS"));
        feedback.setText(juce::String{}, juce::dontSendNotification);
        repaint();
    }
    void timerCallback() override {
        if (panelLanguageId != static_cast<int>(acid303i18n::current())) applyPanelLanguage();
    }
};

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
    // Original TB303.knob model supplied by the user, embedded in V3 binary resources.
    // The face is rotated for visual tracking; separate tick ring retains the host angle.
    auto knob=juce::ImageCache::getFromMemory(BinaryData::TB303_Knob_png,
                                              BinaryData::TB303_Knob_pngSize);
    if (knob.isValid()) {
        const float size=radius*2.35f;
        const juce::Rectangle<float> target(cx-size*0.5f,cy-size*0.5f,size,size);
        const float modelNeedleAngle=3.84f; // dark pointer in the supplied 64 px knob
        g.drawImageTransformed(knob,
            juce::AffineTransform::scale(size/float(knob.getWidth()),size/float(knob.getHeight()))
                .translated(target.getX(),target.getY())
                .rotated(a-modelNeedleAngle,cx,cy));
    } else {
        g.setGradientFill(juce::ColourGradient(silver1,cx-radius,cy-radius,
                                               silver3,cx+radius,cy+radius,false));
        g.fillEllipse(cx-radius,cy-radius,2*radius,2*radius);
        g.setColour(accentColour); g.drawLine(cx,cy,cx+std::sin(a)*radius*0.8f,
                                                cy-std::cos(a)*radius*0.8f,3.f);
    }
}

Acid303SoloEditor::Acid303SoloEditor(Acid303SoloProcessor& proc)
    : AudioProcessorEditor(&proc), processor(proc), sequenceGrid(proc) {
    keepHostKeyboardFocus(*this);
    keepHostKeyboardFocus(sequenceGrid);
    // Explicit base pointer type is necessary: these controls are a mixture of
    // juce::TextButton, juce::ComboBox and juce::Label. MSVC cannot deduce a
    // common initializer_list type from different derived pointer classes.
    const std::array<juce::Component*, 27> keyboardFocusControls{{
        &modeButton, &randomButton, &midiSetupButton, &rateChoice, &rateLabel,
        &savePresetButton, &loadPresetButton, &presetFolderButton, &updateButton,
        &previousPresetButton, &presetNameLabel, &nextPresetButton, &skinLabel, &skinChoice, &languageChoice,
        &exportMidiButton, &importMidiButton, &midiFolderButton,
        &delayModeChoice, &delaySubdivision,
        &prevDistType, &nextDistType, &distTypeName, &tempoLabel,
        &transposeChoice, &transposeLabel, &midiPatternMapButton
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
    distTypeName.setTooltip("Deux modes : TUBE (overdrive acid a diodes ameliore) et BITCRUSHER. Les anciens presets gardent leur choix.");
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
    for (auto* knob : {&patternGroupKnob,&patternNumberKnob}) {
        knob->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        knob->setRotaryParameters(juce::degreesToRadians(225.0f),
                                  juce::degreesToRadians(495.0f),true);
        knob->setRange(0,7,1);
        knob->setTextBoxStyle(juce::Slider::NoTextBox,false,0,0);
        knob->setLookAndFeel(&knobLook);
        knob->setPopupDisplayEnabled(false,false,this);
        addAndMakeVisible(*knob);
        keepHostKeyboardFocus(*knob);
    }
    patternGroupKnob.setTooltip("TB-303 groups: 1A 1B 2A 2B 3A 3B 4A 4B");
    patternNumberKnob.setTooltip("Pattern 1 to 8. 8 groups x 8 patterns = 64.");
    patternGroupKnob.onValueChange=[this] { processor.selectPatternByKnobs(
        juce::roundToInt(patternGroupKnob.getValue()), juce::roundToInt(patternNumberKnob.getValue()));
        sequenceGrid.repaint(); repaint(); };
    patternNumberKnob.onValueChange=[this] { processor.selectPatternByKnobs(
        juce::roundToInt(patternGroupKnob.getValue()), juce::roundToInt(patternNumberKnob.getValue()));
        sequenceGrid.repaint(); repaint(); };
    patternGroupKnob.setValue(processor.getPatternGroup(),juce::dontSendNotification);
    patternNumberKnob.setValue(processor.getPatternNumber(),juce::dontSendNotification);
    bankTriggerButton.setTooltip("Enabled only with SEQUENCER. MIDI notes 36..99 select/loop 64 patterns. OFF: legacy 2.0.6 behavior.");
    bankTriggerButton.onClick=[this] { processor.setBankTriggerEnabled(!processor.getBankTriggerEnabled()); timerCallback(); };
    bankTriggerButton.setColour(juce::TextButton::buttonColourId,blackMetal);
    bankTriggerButton.setColour(juce::TextButton::textColourOffId,neonGreen);
    addAndMakeVisible(bankTriggerButton);
    bankStatusLabel.setJustificationType(juce::Justification::centred);
    bankStatusLabel.setFont(juce::Font(juce::FontOptions(11.0f)));
    addAndMakeVisible(bankStatusLabel);
    transposeLabel.setText("TRANSPOSE", juce::dontSendNotification);
    transposeLabel.setJustificationType(juce::Justification::centred);
    transposeLabel.setFont(juce::Font(juce::FontOptions(12.f).withStyle("Bold")));
    addAndMakeVisible(transposeLabel);
    for (int semitones=-12; semitones<=12; ++semitones)
        transposeChoice.addItem(juce::String(semitones > 0 ? "+" : "") + juce::String(semitones)
            + (semitones == 0 ? " / ORIGINAL" : " DEMI-TONS"), semitones + 13);
    transposeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        processor.params, "bankTranspose", transposeChoice);
    transposeChoice.setTooltip("Transposition globale des 64 patterns, -12 à +12 demi-tons. Exemple : do -> fa = +5. Les notes originales ne sont jamais modifiees.");
    addAndMakeVisible(transposeChoice);
    midiPatternMapButton.setTooltip("Afficher les 64 notes de déclenchement du piano roll et copier toute la table.");
    midiPatternMapButton.onClick = [this] {
        static const juce::StringArray groups = {"1A", "1B", "2A", "2B", "3A", "3B", "4A", "4B"};
        static const juce::StringArray pitches = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
        juce::PopupMenu menu;
        menu.addSectionHeader("PIANO ROLL - MIDI 36..99 (64 patterns)");
        menu.addItem(9999, "COPIER LES 64 CORRESPONDANCES");
        for (int group=0; group<8; ++group) {
            juce::PopupMenu bankMenu;
            for (int number=0; number<8; ++number) {
                const int midi=36+group*8+number;
                bankMenu.addItem(1+group*8+number, "Pattern " + juce::String(number+1)
                    + "  -  MIDI " + juce::String(midi) + " / "
                    + pitches[midi%12] + juce::String(midi/12));
            }
            menu.addSubMenu("GROUPE " + groups[group], bankMenu);
        }
        menu.addSeparator();
        menu.addSectionHeader("La numérotation des octaves peut varier suivant FL Studio.");
        juce::Component::SafePointer<Acid303SoloEditor> safe(this);
        menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&midiPatternMapButton),
                           [safe](int chosen) {
            if (safe == nullptr) return;
            if (chosen == 9999) {
                const juce::StringArray groups2={"1A","1B","2A","2B","3A","3B","4A","4B"};
                const juce::StringArray names={"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};
                juce::String copy="ACID303 SOLO V3 - 64 PATTERNS MIDI 36-99\n";
                for (int slot=0;slot<64;++slot) {
                    const int midi=36+slot;
                    copy += groups2[slot/8]+"-"+juce::String(slot%8+1)+"  MIDI "
                        +juce::String(midi)+" ("+names[midi%12]+juce::String(midi/12)+")\n";
                }
                juce::SystemClipboard::copyTextToClipboard(copy);
            } else if (chosen>=1 && chosen<=64) {
                safe->processor.setSelectedPattern(chosen-1);
                safe->timerCallback();
            }
        });
    };
    addAndMakeVisible(midiPatternMapButton);

    modeButton.setColour(juce::TextButton::buttonColourId, blackMetal);
    modeButton.setColour(juce::TextButton::textColourOffId, neonGreen);
    modeButton.setTooltip("Piano Roll : jouez les notes MIDI. Sequenceur : une note MIDI maintenue declenche les 16 pas, note relachee = stop. Tempo FL Studio.");
    modeButton.onClick = [this] {
        processor.setPlayMode(1 - processor.getPlayMode());
        timerCallback();
    };
    addAndMakeVisible(modeButton);
    randomButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff25482d));
    randomButton.setColour(juce::TextButton::textColourOffId, neonGreen);
    randomButton.setTooltip("Generer 16 pas en gamme mineure avec silences, slides et accents. Le son et les effets restent identiques.");
    randomButton.onClick = [this] {
        processor.randomize16StepPattern();
        sequenceGrid.repaint();
    };
    addAndMakeVisible(randomButton);
    midiSetupButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff122f2b));
    midiSetupButton.setColour(juce::TextButton::textColourOffId, neonGreen);
    midiSetupButton.setTooltip("Affectation MIDI par FL Studio Link (controleurs USB/MIDI).");
    midiSetupButton.onClick = [this] {
        juce::CallOutBox::launchAsynchronously(
            std::make_unique<Acid303MidiSetupPanel>(),
            midiSetupButton.getBounds(), this);
    };
    addAndMakeVisible(midiSetupButton);
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
    previousPresetButton.setTooltip("Preset precedent du dernier dossier utilise (sous-dossiers inclus)");
    nextPresetButton.setTooltip("Preset suivant du dernier dossier utilise (sous-dossiers inclus)");
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
    languageChoice.addItem(juce::String("FRAN") + juce::String::charToString((juce_wchar) 0x00C7) + "AIS", 1);
    languageChoice.addItem("ENGLISH", 2);
    languageChoice.setSelectedId(static_cast<int>(acid303i18n::current()), juce::dontSendNotification);
    languageChoice.onChange = [this] {
        const int lang = languageChoice.getSelectedId();
        if (lang == 1 || lang == 2) {
            acid303i18n::select(static_cast<acid303i18n::Language>(lang));
            applyLanguage();
        }
    };
    addAndMakeVisible(languageChoice);
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
                tr("Dossier des presets", "Preset folder"), result.getErrorMessage());
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
                tr("Dossier MIDI", "MIDI folder"), result.getErrorMessage());
        else Acid303SoloProcessor::getUserMidiFolder().revealToUser();
    };
    ensureFactorySkinFiles();
    Acid303SoloProcessor::installFactoryContentForCurrentUser();
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
    setSize(1152, 720);
    applySkin(getStoredSkinName(processor));
    applyLanguage();
    checkForUpdates();
    // Faster visual feedback for MIDI-triggered pattern changes (20 Hz = 50 ms).
    // The original 8-Hz cadence of background update/skin checks is retained below
    // by scaling their tick thresholds, without adding audio-thread UI calls.
    startTimerHz(20);
    timerCallback();
}
void Acid303SoloEditor::mouseDown(const juce::MouseEvent& event) {
    if (event.y > getHeight() * (66.0f / virtualHeight) ||
        event.x > getWidth() * (620.0f / virtualWidth)) {
        juce::AudioProcessorEditor::mouseDown(event);
        return;
    }
    juce::PopupMenu menu;
    menu.addItem(1, tr("Vérifier les mises à jour GitHub", "Check GitHub updates"), true, false);
    if (updateOffer.newer)
        menu.addItem(2, tr("Télécharger v", "Download v") + updateOffer.version, true, false);
    menu.addItem(3, tr("Ouvrir les versions GitHub", "Open GitHub Releases"), true, false);
    juce::PopupMenu skins;
    for (int i = 0; i < availableSkinNames.size(); ++i)
        skins.addItem(100+i, availableSkinNames[i], true,
                      availableSkinNames[i] == getStoredSkinName(processor));
    menu.addSubMenu(tr("Thèmes", "Skins"), skins);
    juce::PopupMenu languages;
    languages.addItem(201, juce::String("Fran") + juce::String::charToString((juce_wchar) 0x00E7) + "ais", true, acid303i18n::current() == acid303i18n::Language::French);
    languages.addItem(202, "English", true, acid303i18n::current() == acid303i18n::Language::English);
    menu.addSubMenu(tr("Langue", "Language"), languages);
    menu.addSeparator();
    menu.addItem(4, tr("Preset précédent", "Previous preset"));
    menu.addItem(5, tr("Preset suivant", "Next preset"));
    const juce::Component::SafePointer<Acid303SoloEditor> safe(this);
    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(this).withMousePosition(),
                       [safe](int id) {
        if (safe == nullptr) return;
        if (id == 1) safe->checkForUpdates();
        else if (id == 2) safe->handleUpdateClick();
        else if (id == 3) juce::URL("https://github.com/burnouttheory/Acid303-Solo/releases").launchInDefaultBrowser();
        else if (id == 4) safe->stepUserPreset(-1);
        else if (id == 5) safe->stepUserPreset(1);
        else if (id == 201 || id == 202) {
            acid303i18n::select(id == 201 ? acid303i18n::Language::French : acid303i18n::Language::English);
            safe->applyLanguage();
        }
        else if (id >= 100 && id < 100 + safe->availableSkinNames.size()) {
            const auto name = safe->availableSkinNames[id - 100];
            safe->processor.params.state.setProperty(juce::Identifier("selectedSkinName"), name, nullptr);
            safe->applySkin(name);
        }
    });
}

Acid303SoloEditor::~Acid303SoloEditor() {
    updateWorker.signalThreadShouldExit();
    updateWorker.stopThread(6000);
    for (auto& c : controls) if (c.slider) c.slider->setLookAndFeel(nullptr);
    patternGroupKnob.setLookAndFeel(nullptr);
    patternNumberKnob.setLookAndFeel(nullptr);
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
    // Same full-featured toolbar as v0.30.2 for EVERY skin, including V1 ORIGINAL.
    // In particular MAJ, SKINS, prev/next and PRESET actions are always visible.
    // Language beside the tempo and skin selectors, above the synth.
    languageChoice.setBounds(B(582,18,127,30));
    tempoLabel.setBounds(B(715,18,246,30));
    skinLabel.setBounds(B(964,18,68,30));
    skinChoice.setBounds(B(1038,18,133,30));
    // Fixed: RANDOM is to the LEFT of the speed selector, not on top of
    // the sequencer header. These rectangles never overlap.
    randomButton.setBounds(B(195,607,129,28));
    rateLabel.setBounds(B(329,608,71,28));
    rateChoice.setBounds(B(405,607,178,28));
    modeButton.setBounds(B(589,607,267,28));
    midiSetupButton.setBounds(B(862,607,295,28));
    // Patch selector (left/right buttons) and file operations above OSC / FILTER / ENV / OUTPUT.
    previousPresetButton.setBounds(B(42,79,34,31));
    presetNameLabel.setBounds(B(86,79,355,31));
    nextPresetButton.setBounds(B(449,79,34,31));
    savePresetButton.setBounds(B(492,79,133,31));
    loadPresetButton.setBounds(B(634,79,133,31));
    presetFolderButton.setBounds(B(776,79,119,31));
    updateButton.setBounds(B(932,79,225,31));
    sequenceGrid.setBounds(B(38,639,1123,221));
    patternGroupKnob.setBounds(B(1249,259,130,130));
    patternNumberKnob.setBounds(B(1249,480,130,130));
    bankTriggerButton.setBounds(B(1225,664,186,34));
    bankStatusLabel.setBounds(B(1218,703,198,46));
    transposeLabel.setBounds(B(1218,748,198,18));
    transposeChoice.setBounds(B(1225,770,186,33));
    midiPatternMapButton.setBounds(B(1225,818,186,32));
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
    auto paintBankSide = [&] {
        // V3 UI correction 02: panel and printed dial legends occupy distinct,
        // non-overlapping rows. Audio, preset serialization and VST3 IDs untouched.
        const bool grey = theme.name == "TB303 GREY V3";
        const auto panel = juce::Rectangle<float>(1201.f,68.f,226.f,820.f);
        const auto ink = accent;
        const auto subInk = grey ? theme.soft : soft;
        if (grey) paintBrushedSteel(g,panel);
        else { g.setColour(theme.body); g.fillRoundedRectangle(panel,8.f); }
        g.setColour(accent.withAlpha(0.9f));
        g.drawRoundedRectangle(panel.reduced(1.f),8.f,1.5f);

        g.setColour(ink);
        g.setFont(juce::Font(juce::FontOptions(19.f).withStyle("Bold")));
        g.drawText("PATTERN BANK",1214,90,200,29,juce::Justification::centred);
        g.setColour(accent.withAlpha(0.75f));
        g.drawLine(1222.f,123.f,1406.f,123.f,1.f);

        // Explicit selection display makes the two knobs easier to understand.
        const int group=juce::jlimit(0,7,processor.getPatternGroup());
        const int number=juce::jlimit(0,7,processor.getPatternNumber());
        const juce::StringArray groupNames={"1A","1B","2A","2B","3A","3B","4A","4B"};
        const juce::Rectangle<float> selection(1245.f,137.f,138.f,34.f);
        g.setColour(juce::Colour(0xff202925));
        g.fillRoundedRectangle(selection,5.f);
        g.setColour(accent);
        g.drawRoundedRectangle(selection,5.f,1.f);
        g.setFont(juce::Font(juce::FontOptions(16.f).withStyle("Bold")));
        g.drawText(groupNames[group]+"  /  "+juce::String(number+1),
                   selection,juce::Justification::centred);

        g.setColour(ink);
        g.setFont(juce::Font(juce::FontOptions(12.0f).withStyle("Bold")));
        g.drawText("PATTERN GROUP",1216,197,198,22,juce::Justification::centred);
        g.drawText("PATTERN",1216,418,198,22,juce::Justification::centred);

        // These angles exactly match the JUCE Slider rotary arc (225-495 degrees).
        // The radius fits the 226-pixel panel and avoids both headings.
        constexpr float firstDeg=225.f, rangeDeg=270.f;
        for (int k=0;k<8;++k) {
            const float angle=juce::degreesToRadians(firstDeg+rangeDeg*float(k)/7.f);
            const float x=1314.f+86.f*std::sin(angle);
            const float y=324.f-86.f*std::cos(angle);
            g.setColour(k==group ? accent : subInk);
            g.setFont(juce::Font(juce::FontOptions(12.f).withStyle(k==group ? "Bold" : "Regular")));
            g.drawText(groupNames[k],juce::roundToInt(x)-18,juce::roundToInt(y)-12,
                       36,24,juce::Justification::centred);
        }
        for (int k=0;k<8;++k) {
            const float angle=juce::degreesToRadians(firstDeg+rangeDeg*float(k)/7.f);
            const float x=1314.f+86.f*std::sin(angle);
            const float y=545.f-86.f*std::cos(angle);
            g.setColour(k==number ? accent : subInk);
            g.setFont(juce::Font(juce::FontOptions(12.f).withStyle(k==number ? "Bold" : "Regular")));
            g.drawText(juce::String(k+1),juce::roundToInt(x)-14,juce::roundToInt(y)-12,
                       28,24,juce::Justification::centred);
        }
        g.setColour(ink);
        g.setFont(juce::Font(juce::FontOptions(11.f).withStyle("Bold")));
        g.drawText("MIDI 36-99 / 64 PATTERNS",1210,862,211,20,juce::Justification::centred);
        if (grey) paintPanelScrews(g,panel);
    };
    if (theme.name == "V1 ORIGINAL") {
        const auto frame = juce::Rectangle<float>(9.0f,8.0f,1182.0f,884.0f);
        g.setColour(juce::Colour(0xff11191d));
        g.fillRoundedRectangle(frame, 8.0f);
        g.setColour(juce::Colour(0xff35464c));
        g.drawRoundedRectangle(frame, 8.0f, 1.2f);
        g.setColour(juce::Colours::white);
        g.setFont(juce::Font(juce::FontOptions(29.0f).withStyle("Bold")));
        g.drawText("ACID 303",27,19,240,38,juce::Justification::centredLeft);
        g.setFont(juce::Font(juce::FontOptions(12.0f).withStyle("Bold")));
        g.drawText("SOLO V3.0.0 / VST3 / FL STUDIO",263,25,307,22,juce::Justification::centredLeft);
        g.setColour(juce::Colour(0xffa6d8c6));
        // V3 UI fix 04: brand belongs to the free right-hand header area.
        // Never paint it underneath FRANCAIS, FL STUDIO BPM or the skin selector.
        g.drawText("BURNOUT THEORY", 1210, 23, 210, 25,
                   juce::Justification::centred);
        g.setColour(juce::Colour(0xff080e10));
        g.fillRoundedRectangle(854.0f,16.0f,313.0f,36.0f,4.0f);
        g.setColour(juce::Colour(0xff74e6aa));
        // Removed stray green status dot overlapping the BPM value.
        // Adapt the screenshot palette, NOT its omission of the preset/update toolbar.
        // Keep the same geometry and all interactive controls as the v0.30.2 UI.
        g.setColour(juce::Colour(0xff182228));
        g.fillRoundedRectangle(juce::Rectangle<float>(23,68,1154,316),6.0f);
        g.setColour(juce::Colour(0xff52636c));
        g.drawRoundedRectangle(juce::Rectangle<float>(23,68,1154,316),6.0f,1.25f);
        // Dedicated upper toolbar: preset arrows/name, save/load/folder, GitHub MAJ.
        g.setColour(juce::Colour(0xff263239));
        g.fillRoundedRectangle(juce::Rectangle<float>(29,74,872,42),5.0f);
        g.setColour(juce::Colour(0xff566a74));
        g.drawRoundedRectangle(juce::Rectangle<float>(29,74,872,42),5.0f,1.0f);
        g.setColour(juce::Colour(0xff263239));
        g.fillRoundedRectangle(juce::Rectangle<float>(917,74,245,42),5.0f);
        g.setColour(juce::Colour(0xff566a74));
        g.drawRoundedRectangle(juce::Rectangle<float>(917,74,245,42),5.0f,1.0f);
        panel(g,{35,126,333,246},tr("OSCILLATEUR", "OSCILLATOR"),"WAVE / SHAPE / TUNE",false,theme);
        panel(g,{377,126,347,246},tr("FILTRE", "FILTER"),"CUTOFF / RESO / LFO",false,theme);
        panel(g,{733,126,264,246},tr("ENVELOPPE", "ENVELOPE"),"SLIDE / ACCENT",false,theme);
        panel(g,{1006,126,160,246},tr("SORTIE", "OUTPUT"),"MASTER",false,theme);
        panel(g,{23,391,331,198},tr("COMPRESSEUR", "COMPRESSOR"),"3-BAND",true,theme);
        panel(g,{362,391,267,198},tr("DISTORSION", "DISTORTION"),"TUBE / BITCRUSHER",true,theme);
        panel(g,{637,391,255,198},tr("DELAY", "DELAY"),"STEREO / SYNC BPM",true,theme);
        panel(g,{900,391,277,198},tr("REVERB", "REVERB"),"STEREO",true,theme);
        panel(g,{23,600,1154,288},tr("SÉQUENCEUR", "SEQUENCER"),"",false,theme);
        g.setColour(juce::Colours::white);
        g.setFont(juce::Font(juce::FontOptions(11.5f).withStyle("Bold")));
        g.drawText(tr("MIDI : vélocité = accent | chevauchement = slide | import/export : Documents/Acid303/midi",
                      "MIDI: velocity = accent | overlap = slide | import/export: Documents/Acid303/midi"),
                   47,866,752,16,juce::Justification::centredLeft);
        paintBankSide();
        return;
    }
    auto full = juce::Rectangle<float>(8.0f,8.0f,1184.0f,884.0f);
    if (theme.name == "TB303 GREY V3") {
        paintBrushedSteel(g,full);
        paintPanelScrews(g,full);
    } else {
        g.setColour(theme.body);
        g.fillRoundedRectangle(full,8.0f);
        drawCrackTexture(g, full.reduced(4.0f), theme);
    }
    g.setColour(accent.withAlpha(0.9f));
    g.drawRoundedRectangle(full.reduced(0.8f),8.0f,1.6f);

    g.setColour(accent);
    g.setFont(juce::Font(juce::FontOptions(30.0f).withStyle("Bold")));
    g.drawText("ACID 303 V3.0.0",25,17,350,37,juce::Justification::centredLeft);
    g.setFont(juce::Font(juce::FontOptions(13.0f).withStyle("Bold")));
    // V3 UI fix 04: keep the brand in the unused header above PATTERN BANK.
    // Header controls extend through x=1171; the side header starts at x=1201.
    g.drawText("BURNOUT THEORY", 1210, 23, 210, 25,
               juce::Justification::centred);


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
    panel(g,{35,126,333,246},tr("OSCILLATEUR", "OSCILLATOR"),"WAVE / SHAPE / TUNE",false,theme);
    panel(g,{377,126,347,246},tr("FILTRE", "FILTER"),"CUTOFF / RESO / LFO",false,theme);
    panel(g,{733,126,264,246},tr("ENVELOPPE", "ENVELOPE"),"SLIDE / ACCENT",false,theme);
    panel(g,{1006,126,160,246},tr("SORTIE", "OUTPUT"),"MASTER",false,theme);

    panel(g,{23,391,331,198},tr("COMPRESSEUR", "COMPRESSOR"),"3-BAND",true,theme);
    panel(g,{362,391,267,198},tr("DISTORSION", "DISTORTION"),"TUBE / BITCRUSHER",true,theme);
    panel(g,{637,391,255,198},tr("DELAY", "DELAY"),"STEREO / SYNC BPM",true,theme);
    panel(g,{900,391,277,198},tr("REVERB", "REVERB"),"STEREO",true,theme);

    panel(g,{23,600,1154,288},tr("SÉQUENCEUR", "SEQUENCER"),"",false,theme);
    g.setColour(soft);
    g.setFont(juce::Font(juce::FontOptions(12.5f).withStyle("Bold")));
    g.drawText(tr("MIDI : vélocité = accent | chevauchement = slide | import/export : Documents/Acid303/midi",
                      "MIDI: velocity = accent | overlap = slide | import/export: Documents/Acid303/midi"), 
               47,866,752,16,juce::Justification::centredLeft);
    paintBankSide();
}

void Acid303SoloEditor::applySkin(const juce::String& skinName) {
    const auto theme = resolveSkinThemeByName(skinName);
    const auto accent = theme.accent;
    const auto soft = theme.soft;
    const bool lightTheme = theme.panel.getPerceivedBrightness() > 0.65f;
    const bool reference = theme.name == "V1 ORIGINAL";
    knobLook.setAccentColour(reference ? juce::Colour(0xffd98462) : accent);
    for (auto& c : controls) {
        c.slider->setColour(juce::Slider::textBoxBackgroundColourId,
                            reference ? juce::Colour(0xff2a383f) : theme.panel);
        c.slider->setColour(juce::Slider::textBoxTextColourId, reference ? juce::Colours::white : accent);
        c.slider->setColour(juce::Slider::textBoxOutlineColourId,
                            reference ? juce::Colour(0xff566d75) : accent);
        c.slider->setColour(juce::Slider::textBoxHighlightColourId,accent);
        c.label->setColour(juce::Label::textColourId,reference ? juce::Colours::white : accent);
        c.slider->repaint();
    }
    for (auto* label : {&presetNameLabel, &skinLabel, &rateLabel, &tempoLabel,
                        &distTypeName, &delaySubdivision})
        label->setColour(juce::Label::textColourId, accent);
    delaySubdivision.setColour(juce::Label::textColourId, reference ? juce::Colours::white : soft);
    tempoLabel.setColour(juce::Label::textColourId, accent);
    transposeLabel.setColour(juce::Label::textColourId, accent);
    bankStatusLabel.setColour(juce::Label::textColourId, accent);
    distTypeName.setColour(juce::Label::backgroundColourId, theme.panel);
    for (auto* choice : {&rateChoice, &skinChoice, &delayModeChoice, &languageChoice, &transposeChoice}) {
        choice->setColour(juce::ComboBox::backgroundColourId, theme.panel);
        choice->setColour(juce::ComboBox::textColourId, reference ? juce::Colours::white : accent);
        choice->setColour(juce::ComboBox::outlineColourId,
                          reference ? juce::Colour(0xff687c82) : accent);
        choice->setColour(juce::ComboBox::arrowColourId, reference ? juce::Colours::white : accent);
    }
    for (auto* button : {&modeButton, &randomButton, &midiSetupButton, &midiPatternMapButton,
                         &previousPresetButton, &nextPresetButton,
                         &savePresetButton, &loadPresetButton,
                         &presetFolderButton, &updateButton, &exportMidiButton, &importMidiButton,
                         &midiFolderButton, &prevDistType, &nextDistType}) {
        button->setColour(juce::TextButton::buttonColourId, theme.panel);
        button->setColour(juce::TextButton::textColourOffId, reference ? juce::Colours::white : accent);
    }
    if (theme.name != "TB303 GREY V3") {
        randomButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff25482d));
        randomButton.setColour(juce::TextButton::textColourOffId, neonGreen);
        midiSetupButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff122f2b));
        midiSetupButton.setColour(juce::TextButton::textColourOffId, neonGreen);
    }
    if (theme.name == "TB303 GREY V3") {
        // Same dark fill as MODE: PIANO ROLL, instead of the historical green fills.
        for (auto* button : {&modeButton, &randomButton, &midiSetupButton}) {
            button->setColour(juce::TextButton::buttonColourId, theme.panel);
            button->setColour(juce::TextButton::buttonOnColourId, theme.panel);
            button->setColour(juce::TextButton::textColourOffId, theme.accent);
            button->setColour(juce::TextButton::textColourOnId, theme.accent);
        }
    }
    bankTriggerButton.setColour(juce::TextButton::buttonColourId, theme.panel);
    bankTriggerButton.setColour(juce::TextButton::textColourOffId, accent);
    bankStatusLabel.setColour(juce::Label::textColourId,
                              accent);
    // Preserve EVERY v0.30.2 control as a visible, direct-action widget.
    // No V1 skin is allowed to hide preset navigation, skin selection or GitHub MAJ.
    for (auto* button : {&previousPresetButton, &nextPresetButton, &savePresetButton,
                         &loadPresetButton, &presetFolderButton, &updateButton})
        button->setVisible(true);
    presetNameLabel.setVisible(true);
    skinLabel.setVisible(true);
    skinChoice.setVisible(true);
    languageChoice.setVisible(true);
    if (reference) {
        modeButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffffc166));
        distTypeName.setColour(juce::Label::textColourId, juce::Colour(0xffffc166));
        distTypeName.setColour(juce::Label::backgroundColourId, juce::Colour(0xff080f13));
    }
    for (auto& effect : effectButtons) {
        effect.button->setColour(juce::TextButton::buttonColourId,
            reference ? juce::Colour(0xff803440) : lightTheme ? theme.panel.darker(0.09f) : theme.fxGlow);
        effect.button->setColour(juce::TextButton::buttonOnColourId,
            reference ? juce::Colour(0xff328360) : lightTheme ? theme.panel.darker(0.20f) : theme.panel.brighter(0.12f));
        effect.button->setColour(juce::TextButton::textColourOffId, reference ? juce::Colours::white : accent);
        effect.button->setColour(juce::TextButton::textColourOnId, reference ? juce::Colours::white : accent);
    }
    if (updateOffer.newer) updateButton.setColour(juce::TextButton::buttonColourId, theme.fxAccent.darker(0.55f));
    activeSkinName = theme.name;
    resized(); // use the same proven v0.30.2 geometry for every skin
    sequenceGrid.repaint();
    repaint();
}


// UI-only language: no modifications to the synth's automatable parameter IDs.
// The global user preference is shared by all Acid303 Solo instances.
void Acid303SoloEditor::applyLanguage() {
    lastLanguageId = static_cast<int>(acid303i18n::current());
    languageChoice.setSelectedId(lastLanguageId, juce::dontSendNotification);
    languageChoice.setTooltip(tr("Langue de l'interface (préférence utilisateur)",
                                 "Interface language (user preference)"));
    skinLabel.setText(tr("THÈMES", "SKINS"), juce::dontSendNotification);
    skinChoice.setTooltip(tr("Choisir un thème dans Documents/Acid303/skins",
                             "Choose a theme from Documents/Acid303/skins"));
    rateLabel.setText(tr("VITESSE", "SPEED"), juce::dontSendNotification);
    const int selectedRate = juce::jlimit(1, 4, rateChoice.getSelectedId());
    rateChoice.clear(juce::dontSendNotification);
    rateChoice.addItem(tr("NOIRE (1/4)", "QUARTER (1/4)"), 1);
    rateChoice.addItem(tr("CROCHE (1/8)", "EIGHTH (1/8)"), 2);
    rateChoice.addItem(tr("1/16 · DBL CROCHE", "SIXTEENTH (1/16)"), 3);
    rateChoice.addItem(tr("1/32 · TRPL CROCHE", "32ND NOTE (1/32)"), 4);
    rateChoice.setSelectedId(selectedRate, juce::dontSendNotification);
    rateChoice.setTooltip(tr("Durée de chaque pas, synchronisée avec FL Studio.",
                             "Step rate, synchronized with FL Studio."));
    randomButton.setButtonText(tr("ALÉATOIRE 16", "RANDOM 16"));
    randomButton.setTooltip(tr("Générer 16 pas avec notes, silences, slides et accents. Les sons et effets sont conservés.",
                               "Generate 16 steps with notes, rests, slides and accents. Sound and effects stay unchanged."));
    midiSetupButton.setButtonText(tr("CONFIG MIDI", "MIDI SETUP"));
    transposeLabel.setText(tr("TRANSPOSITION", "TRANSPOSE"), juce::dontSendNotification);
    midiPatternMapButton.setButtonText(tr("CARTE MIDI 64", "MIDI MAP 64"));
    midiSetupButton.setTooltip(tr("Affectation MIDI via FL Studio Link.",
                                  "MIDI mapping via FL Studio Link."));
    savePresetButton.setButtonText(tr("SAUVER PRESET", "SAVE PRESET"));
    loadPresetButton.setButtonText(tr("CHARGER PRESET", "LOAD PRESET"));
    presetFolderButton.setButtonText(tr("DOSSIER", "FOLDER"));
    exportMidiButton.setButtonText(tr("EXPORT MIDI", "EXPORT MIDI"));
    importMidiButton.setButtonText(tr("IMPORT MIDI", "IMPORT MIDI"));
    midiFolderButton.setButtonText(tr("DOSSIER MIDI", "MIDI FOLDER"));
    savePresetButton.setTooltip(tr("Enregistrer le son, la transposition et les 64 patterns dans un preset.",
                                   "Save sound settings, transpose and all 64 patterns to a preset."));
    loadPresetButton.setTooltip(tr("Charger un preset Acid303.", "Load an Acid303 preset."));
    presetFolderButton.setTooltip(tr("Ouvrir le dossier des presets.", "Open the presets folder."));
    modeButton.setTooltip(tr("Piano Roll : jouer depuis le clavier MIDI. Séquenceur : maintenir une note pour déclencher le motif.",
                             "Piano Roll: play via MIDI notes. Sequencer: hold a note to run the pattern."));
    updateButton.setTooltip(tr("Rechercher une nouvelle version sur GitHub.", "Check GitHub for updates."));
    prevDistType.setTooltip(tr("Effet de distorsion précédent.", "Previous distortion type."));
    nextDistType.setTooltip(tr("Effet de distorsion suivant.", "Next distortion type."));
    delayModeChoice.setTooltip(tr("NORMAL : écho standard ; PING-PONG : alternance gauche/droite.",
                                  "NORMAL: regular echoes; PING-PONG: alternating left/right."));
    // Most short synth labels (CUTOFF, WAVE, RESO, DRY/WET...) are universal
    // conventions. Only language-specific labels change, not param IDs.
    controls[6].label->setText(tr("VIT. LFO", "LFO RATE"), juce::dontSendNotification);
    controls[11].label->setText(tr("VOL. GÉNÉRAL", "MASTER VOL."), juce::dontSendNotification);
    controls[8].label->setText(tr("GLISSEMENT", "SLIDE TIME"), juce::dontSendNotification);
    controls[18].label->setText(tr("VOLUME", "VOLUME"), juce::dontSendNotification);
    displayUpdateStatus();
    sequenceGrid.repaint();
    repaint();
}

void Acid303SoloEditor::timerCallback() {
    acid303update::Offer checked;
    if (updateWorker.takeResult(checked)) {
        updateOffer = checked;
        displayUpdateStatus();
    }
    if (++updateCheckTicks >= 20 * 3600) { updateCheckTicks = 0; checkForUpdates(); }
    // Look for newly copied or removed external .acid303skin files every 2s.
    // Do not interrupt the user while the dropdown menu is open.
    if (++skinScanTicks >= 40) {
        skinScanTicks = 0;
        if (!skinChoice.isPopupActive()) refreshSkinChoices();
    }
    if (lastLanguageId != static_cast<int>(acid303i18n::current())) applyLanguage();
    const auto selectedSkin = getStoredSkinName(processor);
    if (activeSkinName != selectedSkin) applySkin(selectedSkin);
    if (availableSkinNames.indexOf(selectedSkin) >= 0 && skinChoice.getSelectedItemIndex() != availableSkinNames.indexOf(selectedSkin))
        skinChoice.setSelectedItemIndex(availableSkinNames.indexOf(selectedSkin), juce::dontSendNotification);
    const auto shown = tr("PRESET : ", "PRESET: ") + processor.getCurrentPresetName();
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
    modeButton.setButtonText(processor.getPlayMode() == 0
        ? tr("MODE : PIANO ROLL", "MODE: PIANO ROLL")
        : (processor.currentSequenceStep.load(std::memory_order_relaxed) < 0
            ? tr("SÉQUENCEUR : ATTENTE MIDI", "SEQUENCER: WAITING FOR MIDI")
            : tr("SÉQUENCEUR : ACTIF", "SEQUENCER: ACTIVE")));
    // MIDI BANKS playback chooses a pattern on the audio thread and publishes
    // its slot through an atomic. Only the JUCE MESSAGE THREAD may change the
    // editing selection and its host-notified seqLength parameter. Never call
    // setSelectedPattern or mutate UI directly from processBlock().
    const int playing=processor.currentPlaybackPattern.load(std::memory_order_acquire);
    const bool followMidiPattern = processor.getBankTriggerEnabled() && processor.getPlayMode() == 1;
    bool selectionChanged = false;
    if (followMidiPattern && playing >= 0 && playing < Acid303SoloProcessor::kPatternCount
        && playing != processor.getSelectedPattern()) {
        processor.setSelectedPattern(playing);
        selectionChanged = true;
    }
    // Keep the most recently triggered pattern visible after MIDI Note Off.
    // With MIDI BANKS OFF, the two knobs still follow the manual edit selection.
    const int displayedGroup = processor.getPatternGroup();
    const int displayedNumber = processor.getPatternNumber();
    if (juce::roundToInt(patternGroupKnob.getValue()) != displayedGroup) {
        patternGroupKnob.setValue(displayedGroup,juce::dontSendNotification);
        selectionChanged = true;
    }
    if (juce::roundToInt(patternNumberKnob.getValue()) != displayedNumber) {
        patternNumberKnob.setValue(displayedNumber,juce::dontSendNotification);
        selectionChanged = true;
    }
    if (selectionChanged)
        repaint(); // The selection rectangle in paintBankSide is not a child Label.
    bankTriggerButton.setButtonText(processor.getBankTriggerEnabled() ? "MIDI BANKS : ON" : "MIDI BANKS : OFF");
    const int selectedPattern=processor.getSelectedPattern();
    bankStatusLabel.setText("EDIT: "+juce::String(selectedPattern/16+1)+
        ((selectedPattern/8)%2==0 ? "A" : "B")+" / "+juce::String(selectedPattern%8+1)+
        " | MIDI " + (playing>=0 ? juce::String(playing+36) : "--"),juce::dontSendNotification);
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
        updateButton.setButtonText(tr("MAJ : NON CONFIGURÉE", "UPDATE: NOT CONFIGURED"));
        updateButton.setTooltip(tr("L’éditeur doit configurer l’URL de latest.json.", "Publisher must configure the latest.json URL."));
    } else if (!updateOffer.valid) {
        updateButton.setButtonText(tr("MAJ : HORS LIGNE", "UPDATE: OFFLINE"));
        updateButton.setTooltip(updateOffer.error + tr("\nCliquez pour réessayer.", "\nClick to retry."));
    } else if (updateOffer.newer) {
        updateButton.setButtonText(tr("MAJ : v", "UPDATE: v") + updateOffer.version + tr(" DISPONIBLE", " AVAILABLE"));
        updateButton.setTooltip(updateOffer.notes + tr("\nCliquez pour ouvrir GitHub Releases.", "\nClick to open GitHub Releases."));
    } else {
        updateButton.setButtonText(tr("À JOUR : v", "UP TO DATE: v") + acid303update::installedVersion());
        updateButton.setTooltip(tr("Version à jour. Cliquez pour rechercher des nouveautés.", "Version is current. Click to check for updates."));
    }
    updateButton.setColour(juce::TextButton::buttonColourId,
                           updateOffer.newer ? juce::Colour(0xff5d2512) : juce::Colour(0xff06110a));
    // Always display status, including "A JOUR" on the V1 ORIGINAL skin.
    updateButton.setVisible(true);
}

void Acid303SoloEditor::handleUpdateClick() {
    if (updateOffer.valid && updateOffer.newer) {
        juce::URL(updateOffer.downloadUrl).launchInDefaultBrowser();
        return;
    }
    updateButton.setButtonText(tr("MAJ : VÉRIFICATION...", "CHECKING UPDATE..."));
    checkForUpdates();
}

void Acid303SoloEditor::chooseMidiFile(bool saving) {
    if (auto result = Acid303SoloProcessor::prepareMidiFolder(); result.failed()) {
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
            tr("Dossier MIDI", "MIDI folder"), result.getErrorMessage());
        return;
    }
    const auto folder = Acid303SoloProcessor::getUserMidiFolder();
    const auto suggested = saving ? folder.getChildFile("Ma sequence.mid") : folder;
    midiChooser = std::make_unique<juce::FileChooser>(
        saving ? tr("Exporter une séquence MIDI Acid303", "Export Acid303 MIDI sequence") : tr("Importer une séquence MIDI", "Import MIDI sequence"),
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
                tr("MIDI importé", "MIDI imported"),
                tr("Les 16 pas ont été actualisés.\nUn MIDI externe peut perdre des données propres à Acid303.",
                   "The 16 steps were updated.\nExternal MIDI files may omit Acid303-specific information."));
        }
    });
}

// A host-visible choice parameter, not a UI-only switch. This is saved with
// FL Studio projects and can also be automated from FL Studio.
void Acid303SoloEditor::stepDistType(int delta) {
    auto* parameter = processor.params.getParameter("distType");
    if (parameter == nullptr) return;
    const int current = juce::jlimit(0,1,static_cast<int>(
        processor.params.getRawParameterValue("distType")->load(std::memory_order_relaxed) + 0.5f));
    const int next = (current + delta % 2 + 2) % 2;
    parameter->beginChangeGesture();
    parameter->setValueNotifyingHost(parameter->convertTo0to1(static_cast<float>(next)));
    parameter->endChangeGesture();
    timerCallback();
}

void Acid303SoloEditor::stepUserPreset(int direction) {
    if (auto result = Acid303SoloProcessor::preparePresetFolder(); result.failed()) {
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
            "Presets", result.getErrorMessage());
        return;
    }

    // Navigate the current preset's directory when one was opened/saved in
    // this editor, otherwise use the last user-wide directory. Recursion lets
    // the arrows work from presets/ itself and from Factory/ or User/ without
    // requiring the user to move presets into the root directory.
    const auto root = Acid303SoloProcessor::getUserPresetFolder();
    auto folder = lastSelectedPreset.existsAsFile()
        ? lastSelectedPreset.getParentDirectory() : getLastPresetDirectory();
    if (!folder.isDirectory()) folder = root;

    auto collect = [](const juce::File& directory) {
        std::vector<juce::File> result;
        for (const auto& file : directory.findChildFiles(
                 juce::File::findFiles, true, "*.acid303preset"))
            result.push_back(file);
        // A deterministic order, even if two different subfolders contain
        // presets with the same name. No repeated or skipped files.
        std::sort(result.begin(), result.end(), [&directory](const juce::File& a,
                                                             const juce::File& b) {
            return a.getRelativePathFrom(directory).compareIgnoreCase(
                       b.getRelativePathFrom(directory)) < 0;
        });
        return result;
    };
    auto files = collect(folder);
    if (files.empty() && folder != root) {
        folder = root;
        files = collect(root);
    }
    if (files.empty()) {
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon, "Presets",
            tr("Aucun fichier .acid303preset dans ce dossier ni ses sous-dossiers.",
               "No .acid303preset files in this folder or its subfolders."));
        return;
    }

    int currentIndex = -1;
    // Exact file path takes precedence over display name: two categories can
    // contain presets with identical file names.
    if (lastSelectedPreset.existsAsFile()) {
        for (int i = 0; i < static_cast<int>(files.size()); ++i)
            if (files[static_cast<size_t>(i)] == lastSelectedPreset) {
                currentIndex = i;
                break;
            }
    }
    if (currentIndex < 0) {
        const auto name = processor.getCurrentPresetName();
        for (int i = 0; i < static_cast<int>(files.size()); ++i)
            if (files[static_cast<size_t>(i)].getFileNameWithoutExtension().equalsIgnoreCase(name)) {
                currentIndex = i;
                break;
            }
    }
    const auto n = static_cast<int>(files.size());
    const auto chosenIndex = currentIndex < 0 ? (direction > 0 ? 0 : n - 1)
        : (currentIndex + (direction > 0 ? 1 : -1) + n) % n;
    const auto chosenFile = files[static_cast<size_t>(chosenIndex)];
    if (auto result = processor.loadPresetFromFile(chosenFile); result.failed()) {
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
            "Preset Acid303", result.getErrorMessage());
        return;
    }
    lastSelectedPreset = chosenFile;
    rememberLastPresetDirectory(chosenFile.getParentDirectory());
    timerCallback();
    repaint();
}

void Acid303SoloEditor::choosePresetFile(bool saving) {
    if (auto result = Acid303SoloProcessor::preparePresetFolder(); result.failed()) {
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
            tr("Dossier des presets", "Preset folder"), result.getErrorMessage());
        return;
    }
    const auto folder = getLastPresetDirectory();
    juce::File suggested = folder;
    if (saving) {
        auto currentName = processor.getCurrentPresetName().trim();
        if (currentName.isEmpty() || currentName.equalsIgnoreCase("INIT PATCH"))
            currentName = "Mon preset";
        const auto legalName = juce::File::createLegalFileName(currentName);
        suggested = folder.getChildFile((legalName.isNotEmpty() ? legalName : juce::String("Mon preset"))
                                        + ".acid303preset");
    }
    // Native Windows dialog is retained for both Open and Save As.
    presetChooser = std::make_unique<juce::FileChooser>(
        saving ? tr("Enregistrer un preset Acid303", "Save Acid303 preset")
               : tr("Charger un preset Acid303", "Load Acid303 preset"),
        suggested, "*.acid303preset", true);
    const auto flags = saving
        ? (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles
           | juce::FileBrowserComponent::warnAboutOverwriting)
        : (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles);
    juce::Component::SafePointer<Acid303SoloEditor> safe(this);
    presetChooser->launchAsync(flags, [safe, saving](const juce::FileChooser& chooser) {
        if (safe == nullptr) return;
        const auto selected = chooser.getResult();
        if (selected == juce::File()) return; // Cancel: no folder update.
        const auto actualFile = selected.hasFileExtension(".acid303preset")
            ? selected : selected.withFileExtension(".acid303preset");
        const auto result = saving
            ? safe->processor.savePresetToFile(actualFile)
            : safe->processor.loadPresetFromFile(actualFile);
        if (result.failed()) {
            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                "Preset Acid303", result.getErrorMessage());
            return; // An invalid load must not change the remembered folder.
        }
        safe->lastSelectedPreset = actualFile;
        rememberLastPresetDirectory(actualFile.getParentDirectory());
        if (saving) {
            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon,
                tr("Preset enregistré", "Preset saved"),
                tr("Preset sauvegardé dans :\n", "Preset saved to:\n") + actualFile.getFullPathName());
        } else {
            safe->timerCallback();
            safe->repaint();
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
    const bool reference = theme.name == "V1 ORIGINAL";
    const float scaleX = float(getWidth()) / 1123.0f;
    const float scaleY = float(getHeight()) / 221.0f;
    g.addTransform(juce::AffineTransform::scale(scaleX,scaleY));
    const auto bounds = juce::Rectangle<int>(0,0,1123,221);
    g.setColour(reference ? juce::Colour(0xff131b1f) : lightTheme ? theme.panel : juce::Colour(0xff101519));
    g.fillRoundedRectangle(bounds.toFloat(), 4.0f);
    const float cell = float(1123 - labelWidth) / 16.0f;
    const int playing = processor.getPlayMode() == 1
        ? processor.currentSequenceStep.load(std::memory_order_relaxed) : -1;
    // Display order is reversed compared with the original grid.
    // Model data and audio behavior are unchanged.
    const std::array<juce::String,4> rows = {
        "ACCENT", "SLIDE", "NOTE", tr("ON / OFF", "ON / OFF")
    };

    // Step numbers and a selectable LED per column choose the LOOP LENGTH.
    // The first pad at the left is step 1, and numbering grows to step 16
    // at the right, using exactly the same column boundaries as the pads.
    g.setColour(reference ? juce::Colour(0xff141c20) : lightTheme ? theme.body : juce::Colour(0xff151c20));
    g.fillRoundedRectangle(juce::Rectangle<float>(0.0f, 0.0f,
                           1123.0f, float(headerHeight)), 3.0f);
    g.setColour(reference ? juce::Colours::white : accent);
    g.setFont(juce::Font(juce::FontOptions(14.0f).withStyle("Bold")));
    g.drawText(tr("BOUCLE", "LOOP"), 10, 1, labelWidth-20, 19, juce::Justification::centredLeft);
    g.setColour(reference ? juce::Colour(0xffffc063) : soft);
    g.drawText(juce::String(processor.getSequenceLength()) + " / 16", 10, 19, labelWidth-20, 20,
               juce::Justification::centredLeft);
    const int loopLength = processor.getSequenceLength();
    for (int col=0; col<16; ++col) {
        const int left = labelWidth + juce::roundToInt(col * cell);
        const int right = labelWidth + juce::roundToInt((col + 1) * cell);
        const auto hdr = juce::Rectangle<int>(left, 0, right-left, headerHeight);
        if (col == playing) {
            g.setColour((reference ? juce::Colour(0xffffbb65) : accent).withAlpha(0.20f));
            g.fillRect(hdr.reduced(2, 2));
            g.setColour(accent.withAlpha(0.10f));
            g.fillRect(left, headerHeight, right-left, rowHeight*4);
        }
        // Bars every four beats give a clear left-to-right reading.
        if (col % 4 == 0) {
            g.setColour(accent.withAlpha(0.35f));
            g.fillRect(left, 2, 1, headerHeight-4);
        }
        g.setColour(reference ? juce::Colours::white : col < loopLength ? accent : theme.dim);
        g.setFont(juce::Font(juce::FontOptions(13.0f).withStyle("Bold")));
        g.drawText(juce::String(col + 1), juce::Rectangle<int>(left, 0, right-left, 19), juce::Justification::centred);
        const float ledX = 0.5f * float(left + right) - 7.0f;
        g.setColour(juce::Colour(0xff080f12));
        g.fillEllipse(ledX - 1.5f, 24.0f, 17.0f, 17.0f);
        // Orange: selected loop endpoint, green: within loop, grey: excluded.
        g.setColour(reference ? (col + 1 == loopLength ? juce::Colour(0xffffc065) :
                                  col + 1 < loopLength ? juce::Colour(0xff76e7ae) : juce::Colour(0xff28353b)) :
                    (col + 1 == loopLength ? juce::Colour(0xfffb355a) :
                     col + 1 < loopLength ? accent : juce::Colour(0xff233128)));
        g.fillEllipse(ledX, 25.5f, 14.0f, 14.0f);
    }
    for (int row=0; row<4; ++row) {
        const int top=headerHeight+row*rowHeight;
        g.setColour(reference ? (row % 2 == 0 ? juce::Colour(0xff2a373d) : juce::Colour(0xff26343a)) :
            lightTheme ? (row%2 == 0 ? theme.panel : theme.body) :
            (row%2 == 0 ? juce::Colour(0xff06100a) : juce::Colour(0xff040b07)));
        g.fillRect(0,top,labelWidth-1,rowHeight-2);
        g.setColour(reference ? juce::Colours::white : accent);
        g.drawText(rows[static_cast<size_t>(row)], 10,top, labelWidth-20,rowHeight-1,
                   juce::Justification::centredLeft);
        for (int col=0; col<16; ++col) {
            const auto step=processor.getStep(col);
            const int left=labelWidth+juce::roundToInt(col*cell);
            const int right=labelWidth+juce::roundToInt((col+1)*cell);
            juce::Rectangle<float> pad(float(left+4),float(top+6),float(right-left-8),float(rowHeight-12));
            const bool isOn = row==0 ? step.accent : row==1 ? step.slide : row==3 ? step.on : true;
            const auto onColour = reference
                ? (row==0 ? juce::Colour(0xffb97352) : row==1 ? juce::Colour(0xff286a91)
                          : row==2 ? juce::Colour(0xff080d10) : juce::Colour(0xff247657))
                : (row==0 ? juce::Colour(0xff8f1c33) : row==1 ? juce::Colour(0xff0c4826)
                          : row==2 ? juce::Colour(0xff0a0f0d) : juce::Colour(0xff0f3517));
            g.setColour(reference ? (isOn ? onColour : juce::Colour(0xff29373e)) :
                        lightTheme ? (isOn ? theme.panel.darker(0.11f) : theme.panel) :
                        (col < loopLength ? (isOn ? onColour : juce::Colour(0xff0d1510))
                                            : juce::Colour(0xff0a0e0b)));
            g.fillRoundedRectangle(pad,4.0f);
            if (row == 2) {
                // The pitch is ALWAYS white on a very dark pad: never yellow text
                // on amber; label stays readable even at the minimum window size.
                g.setColour(reference ? juce::Colour(0xffffbc6b) : accent);
                g.drawRoundedRectangle(pad.reduced(0.8f),3.0f,1.6f);
                g.setColour(reference ? juce::Colours::white : col < loopLength ? soft : theme.dim.brighter(0.15f));
                g.setFont(juce::Font(juce::FontOptions(16.0f).withStyle("Bold")));
                g.drawText(gridNoteName(step.note), pad.toNearestInt(), juce::Justification::centred);
            } else if (isOn) {
                g.setColour(reference ? juce::Colours::white : soft);
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
