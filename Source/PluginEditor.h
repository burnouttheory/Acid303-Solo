#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "WebUpdate.h"
#include "I18n.h"
#include <vector>
#include <functional>

class VintageKnobLook final : public juce::LookAndFeel_V4 {
public:
    void setAccentColour(juce::Colour c) { accentColour = c; }
    void drawRotarySlider(juce::Graphics& g, int x, int y, int w, int h,
                          float position, float startAngle, float endAngle, juce::Slider&) override;
private:
    juce::Colour accentColour{0xff4cff72};
};

// JUCE Slider with a dedicated right-click action to request the host's
// parameter context menu (VST3 hosts such as FL Studio may offer automation).
class AutomationSlider final : public juce::Slider {
public:
    std::function<void(const juce::MouseEvent&)> onRightClick;
    void mouseDown(const juce::MouseEvent& event) override {
        if (event.mods.isRightButtonDown()) {
            if (onRightClick) onRightClick(event);
            return;
        }
        juce::Slider::mouseDown(event);
    }
};

// Four-row grid, top to bottom: accent / slide / pitch selector / ON.
class Acid303SequenceGrid final : public juce::Component {
public:
    explicit Acid303SequenceGrid(Acid303SoloProcessor& owner) : processor(owner) {}
    void paint(juce::Graphics&) override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
private:
    Acid303SoloProcessor& processor;
    static constexpr int labelWidth = 132;
    static constexpr int headerHeight = 44;
    static constexpr int rowHeight = 44;
    void showNoteMenu(int step);
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Acid303SequenceGrid)
};

class Acid303SoloEditor final : public juce::AudioProcessorEditor, private juce::Timer {
public:
    explicit Acid303SoloEditor(Acid303SoloProcessor&);
    ~Acid303SoloEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& event) override;
private:
    Acid303SoloProcessor& processor;
    VintageKnobLook knobLook;
    Acid303SequenceGrid sequenceGrid;
    juce::TextButton modeButton{"MODE : PIANO ROLL"};
    juce::Slider patternGroupKnob, patternNumberKnob;
    juce::TextButton bankTriggerButton{"MIDI BANKS : OFF"};
    juce::Label bankStatusLabel;
    juce::ComboBox transposeChoice;
    juce::Label transposeLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> transposeAttachment;
    juce::TextButton midiPatternMapButton{"CARTE MIDI"};

    juce::TextButton randomButton{"RANDOM 16"};
    juce::TextButton midiSetupButton{"MIDI CONTROLLER"};
    juce::ComboBox rateChoice;
    juce::Label rateLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> rateAttachment;
    juce::TextButton savePresetButton{"SAVE PRESET"};
    juce::TextButton loadPresetButton{"LOAD PRESET"};
    juce::TextButton presetFolderButton{"DOSSIER"};
    juce::TextButton updateButton{"MAJ : VERIFICATION"};
    acid303update::Offer updateOffer;
    acid303update::CheckThread updateWorker;
    int updateCheckTicks = 0;
    void checkForUpdates();
    void displayUpdateStatus();
    void handleUpdateClick();
    juce::TextButton previousPresetButton{"<"};
    juce::Label presetNameLabel;
    juce::TextButton nextPresetButton{">"};
    juce::Label skinLabel;
    juce::ComboBox skinChoice;
    juce::ComboBox languageChoice;
    int lastLanguageId = 0;
    void applyLanguage();
    juce::StringArray availableSkinNames;
    int skinScanTicks = 0;
    std::unique_ptr<juce::FileChooser> presetChooser;
    juce::File lastSelectedPreset; // Selection unique to this plugin instance.
    void choosePresetFile(bool saving);
    void stepUserPreset(int direction);
    juce::TextButton exportMidiButton{"EXPORT MIDI"};
    juce::TextButton importMidiButton{"IMPORT MIDI"};
    juce::TextButton midiFolderButton{"DOSSIER MIDI"};
    std::unique_ptr<juce::FileChooser> midiChooser;
    void chooseMidiFile(bool saving);
    struct Control {
        std::unique_ptr<AutomationSlider> slider;
        std::unique_ptr<juce::Label> label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attach;
    };
    std::vector<Control> controls;
    struct EffectButton {
        std::unique_ptr<juce::TextButton> button;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> attach;
    };
    std::vector<EffectButton> effectButtons;
    void addEffectButton(const char* id);
    juce::Label tempoLabel;
    juce::Label delaySubdivision;
    juce::ComboBox delayModeChoice;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> delayModeAttachment;
    juce::TextButton prevDistType{"<"}, nextDistType{">"};
    juce::Label distTypeName;
    void stepDistType(int delta);
    void addControl(const char* id, const char* title);
    void arrange(int index, int x, int y, int w=82, int h=99);
    void timerCallback() override;
    void applySkin(const juce::String& skinName);
    void refreshSkinChoices();
    void ensureFactorySkinFiles();
    juce::String activeSkinName;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Acid303SoloEditor)
};
