#pragma once
#include <JuceHeader.h>
#include "Engine.h"
#include "Sequencer.h"
#include "SequenceGate.h"
#include "RandomPattern.h"
#include <atomic>
#include <array>
#include <limits>

class Acid303SoloProcessor final : public juce::AudioProcessor {
public:
    Acid303SoloProcessor();
    ~Acid303SoloProcessor() override;
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    bool isBusesLayoutSupported(const BusesLayout&) const override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Acid303 Solo"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 12.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return "Default"; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*,int) override;

    juce::AudioProcessorValueTreeState params;
    static juce::AudioProcessorValueTreeState::ParameterLayout makeParameters();
    std::atomic<float> hostBpm{120.0f};
    std::atomic<bool> hostTempoAvailable{false};
    std::atomic<int> currentSequenceStep{-1};
    static constexpr int kPatternCount = 64;
    static constexpr int kTriggerMidiFirst = 36;
    static constexpr int kTriggerMidiLast = 99;
    acid303::seq::Step getStep(int index) const;
    acid303::seq::Step getStepForPattern(int pattern, int index) const;
    int getSelectedPattern() const;
    void setSelectedPattern(int index);
    int getPatternGroup() const;
    int getPatternNumber() const;
    void selectPatternByKnobs(int group, int number);
    bool getBankTriggerEnabled() const;
    int getPatternTranspose() const;
    void setBankTriggerEnabled(bool enabled);
    std::atomic<int> currentPlaybackPattern{-1};

    void setStep(int index, acid303::seq::Step step);
    void randomize16StepPattern(uint32_t seed);
    void randomize16StepPattern();
    int getPlayMode() const;
    void setPlayMode(int mode);
    int getSequenceLength() const;
    void setSequenceLength(int length);
    juce::String getCurrentPresetName() const;
    void setCurrentPresetName(const juce::String& name);
    // Complete self-contained preset: sound parameters + all 16 programmed steps.
    juce::Result savePresetToFile(const juce::File& target);
    juce::Result loadPresetFromFile(const juce::File& source);
    static juce::File getBundlePresetFolder();
    static juce::File getUserPresetFolder();
    static juce::File getDefaultPresetFolder();
    static juce::Result preparePresetFolder();
    // Shareable Standard MIDI Files. Keep proprietary metadata for lossless Acid303 round-trips.
    juce::Result exportMidiToFile(const juce::File& target) const;
    juce::Result importMidiFromFile(const juce::File& source);
    static juce::File getUserMidiFolder();
    static juce::Result prepareMidiFolder();
    static juce::File getUserSkinFolder();
    static juce::Result prepareSkinFolder();
    static void installFactoryContentForCurrentUser();
private:
    acid303::Engine engine;
    std::array<std::array<std::atomic<uint32_t>,16>,kPatternCount> bankSteps{};
    std::atomic<int> selectedEditPattern{0};
    std::array<std::atomic<int>,kPatternCount> bankLengths{};
    // Audio-thread-only trigger order: newest held MIDI note wins.
    std::array<uint16_t,kPatternCount> bankHeldCounts{};
    std::array<uint32_t,kPatternCount> bankNoteOrder{};
    uint32_t bankNoteCounter = 0;
    int triggeredBankPattern = -1;

    int lastMode = -1;
    int64_t lastGlobalStep = std::numeric_limits<int64_t>::min();
    int heldSequenceNote = -1;
    bool lastTransportRunning = false;
    acid303::seq::SequenceGate midiGate;
    bool haveLastPpqEnd = false;
    double lastPpqEnd = 0.0;
    double sampleRateHz = 44100.0;
    void triggerSequenceStep(int index, const acid303::Controls& controls);
    void stopSequence();
    void releaseSequenceVoice();
    void clearBankMidiNotes();
    void selectPlayingBankPattern(int index, int seqSpeed, const acid303::Controls& controls);
    void onBankNoteOn(int midiNote, int seqSpeed, const acid303::Controls& controls);
    void onBankNoteOff(int midiNote, int seqSpeed, const acid303::Controls& controls);
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Acid303SoloProcessor)
};
