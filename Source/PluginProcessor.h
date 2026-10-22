#pragma once
#include <JuceHeader.h>
#include "Engine.h"
#include "Sequencer.h"
#include "SequenceGate.h"
#include <atomic>
#include <array>
#include <limits>

class Acid303SoloProcessor final : public juce::AudioProcessor {
public:
    Acid303SoloProcessor();
    ~Acid303SoloProcessor() override = default;
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
    acid303::seq::Step getStep(int index) const;
    void setStep(int index, acid303::seq::Step step);
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
private:
    acid303::Engine engine;
    std::array<std::atomic<uint32_t>, 16> steps{};
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
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Acid303SoloProcessor)
};
