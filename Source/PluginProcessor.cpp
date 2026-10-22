#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "MidiPattern.h"
#include <cmath>
#include <limits>

Acid303SoloProcessor::Acid303SoloProcessor()
  : juce::AudioProcessor(BusesProperties().withOutput("Stereo Out",juce::AudioChannelSet::stereo(),true)),
    params(*this,nullptr,"ACID303_SOLO", makeParameters()) {
    for (int i = 0; i < 16; ++i) {
        acid303::seq::Step step;
        step.on = (i == 0 || i == 3 || i == 4 || i == 7 || i == 8 || i == 11 || i == 12 || i == 15);
        step.note = static_cast<uint8_t>(48 + (i % 4 == 0 ? 0 : i % 4 == 3 ? -2 : 3));
        step.slide = i == 7 || i == 15;
        step.accent = i % 4 == 0;
        steps[static_cast<size_t>(i)].store(acid303::seq::pack(step), std::memory_order_relaxed);
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout Acid303SoloProcessor::makeParameters() {
    juce::AudioProcessorValueTreeState::ParameterLayout p;
    auto add = [&p](const char* id, const char* title, float low, float high, float initial, float interval=0.001f) {
        p.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{id,1},title,
            juce::NormalisableRange<float>(low,high,interval),initial));
    };
    add("wave", "Wave (Saw to Pulse)", 0,1,0);
    add("shape", "Shape / Pulse Width", 0,1,0.5f);
    add("tune", "Tune Semitones", -12,12,0,0.01f);
    add("cutoff", "Filter Cutoff", 0,1,0.38f);
    add("mod", "LFO Modulation Depth", 0,1,0.0f);
    add("reso", "Filter Resonance", 0,1,0.72f);
    add("lfoRate", "LFO Rate Hz", 0.05f,12.0f,1.0f,0.01f);
    add("envMod", "Filter Envelope Amount", 0,1,0.70f);
    add("slideTime", "Legato Slide Time Seconds", 0.005f,0.5f,0.11f,0.001f);
    add("decay", "Decay", 0,1,0.36f);
    add("accent", "Velocity Accent Amount", 0,1,0.68f);
    add("volume", "303 Volume", 0,1,0.70f);
    add("masterVolume", "Volume general - Master", 0,1,0.85f);
    // New V0.19 parameters: stable IDs for independent FL Studio automation.
    p.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"compOn",1},"Compressor Enabled",false));
    add("compLow", "Compressor Low Band", 0,1,0.48f);
    add("compMid", "Compressor Mid Band", 0,1,0.48f);
    add("compHigh", "Compressor High Band", 0,1,0.48f);
    add("compGain", "Compressor Output Gain", 0,2,1.0f);
    add("compWet", "Compressor Dry Wet", 0,1,0.75f);
    p.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"distOn",1},"Distortion Enabled",false));
    add("distWet", "Distortion Dry Wet", 0,1,0.72f);
    add("distDrive", "Distortion Drive", 0,1,0.58f);
    add("distVolume", "Distortion Output Level", 0,1,0.85f);
    add("distEq", "Distortion EQ Tone", 0,1,0.5f);
    p.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{"distType",1}, "Distortion Type",
        juce::StringArray{"Tube", "Bitcrusher"}, 0));
    p.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"delayOn",1},"Delay Enabled",false));
    add("delayWet", "Delay Dry Wet", 0,1,0.30f);
    add("delayTime", "Delay Time Synced", 0,1,0.375f);
    add("delayFeedback", "Delay Feedback", 0,0.89f,0.22f);
    p.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"reverbOn",1},"Reverb Enabled",false));
    add("reverbWet", "Reverb Dry Wet", 0,1,0.40f);
    add("reverbSize", "Reverb Size", 0,1,0.42f);
    add("reverbVolume", "Reverb Volume", 0,1,0.45f);
    p.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{"seqRate",1}, "Sequence Step Duration",
        juce::StringArray{"Noire 1/4", "Croche 1/8", "Double croche 1/16", "Triple croche 1/32"}, 2));
    p.add(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID{"seqLength",1}, "Sequence Length (1-16)", 1, 16, 16));
    p.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{"playMode",1}, "Play Mode",
        juce::StringArray{"Piano Roll", "Sequencer"}, 0));
    // Added in v0.23; append without renaming existing parameter IDs.
    p.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{"delayMode",1}, "Delay Mode",
        juce::StringArray{"Normal", "Ping-Pong"}, 0));
    add("delayWidth", "Delay Stereo Width", 0,1,0.55f);
    // UI-only choice. Appended to retain all existing VST3 parameter IDs.
    p.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{"uiSkin",1}, "Interface Skin",
        juce::StringArray{"Neon Green", "Ice Blue"}, 0));
    return p;
}
void Acid303SoloProcessor::prepareToPlay(double sampleRate, int) {
    sampleRateHz = sampleRate;
    engine.prepare(sampleRate);
    lastMode = -1;
    lastGlobalStep = std::numeric_limits<int64_t>::min();
    lastTransportRunning = false;
    heldSequenceNote = -1;
    midiGate.reset();
    haveLastPpqEnd = false;
    currentSequenceStep.store(-1, std::memory_order_relaxed);
}
acid303::seq::Step Acid303SoloProcessor::getStep(int index) const {
    if (index < 0 || index >= 16) return {};
    return acid303::seq::unpack(steps[static_cast<size_t>(index)].load(std::memory_order_relaxed));
}
void Acid303SoloProcessor::setStep(int index, acid303::seq::Step step) {
    if (index < 0 || index >= 16) return;
    steps[static_cast<size_t>(index)].store(acid303::seq::pack(step), std::memory_order_relaxed);
    // Ask host to mark project as changed so FL Studio will save the edited grid.
    updateHostDisplay(juce::AudioProcessor::ChangeDetails{}.withNonParameterStateChanged(true));
}
int Acid303SoloProcessor::getPlayMode() const {
    return params.getRawParameterValue("playMode")->load(std::memory_order_relaxed) >= 0.5f ? 1 : 0;
}
void Acid303SoloProcessor::setPlayMode(int mode) {
    if (auto* parameter = params.getParameter("playMode")) {
        parameter->beginChangeGesture();
        parameter->setValueNotifyingHost(parameter->convertTo0to1(static_cast<float>(mode != 0 ? 1 : 0)));
        parameter->endChangeGesture();
    }
}
int Acid303SoloProcessor::getSequenceLength() const {
    return juce::jlimit(1,16,static_cast<int>(params.getRawParameterValue("seqLength")->load(std::memory_order_relaxed) + 0.5f));
}
void Acid303SoloProcessor::setSequenceLength(int length) {
    if (auto* param = params.getParameter("seqLength")) {
        param->beginChangeGesture();
        param->setValueNotifyingHost(param->convertTo0to1(static_cast<float>(juce::jlimit(1,16,length))));
        param->endChangeGesture();
    }
}
void Acid303SoloProcessor::releaseSequenceVoice() {
    engine.allNotesOff();
    heldSequenceNote = -1;
    currentSequenceStep.store(-1, std::memory_order_relaxed);
}
void Acid303SoloProcessor::stopSequence() {
    releaseSequenceVoice();
    midiGate.reset();
    lastGlobalStep = std::numeric_limits<int64_t>::min();
}
void Acid303SoloProcessor::triggerSequenceStep(int index, const acid303::Controls& c) {
    const auto next = getStep(index);
    if (!next.on) {
        if (heldSequenceNote >= 0) engine.noteOff(heldSequenceNote, c);
        heldSequenceNote = -1;
        return;
    }
    const int note = static_cast<int>(next.note);
    const float velocity = acid303::seq::accentVelocity(next.accent);
    if (next.slide && heldSequenceNote >= 0) {
        // Trigger before releasing the old note, creating genuine legato glide.
        // On the same pitch, keep the gate open (tie).
        if (note != heldSequenceNote) {
            engine.noteOn(note, velocity, c);
            engine.noteOff(heldSequenceNote, c);
        }
    } else {
        if (heldSequenceNote >= 0) engine.noteOff(heldSequenceNote, c);
        engine.noteOn(note, velocity, c);
    }
    heldSequenceNote = note;
}
bool Acid303SoloProcessor::isBusesLayoutSupported(const BusesLayout& buses) const {
    return buses.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void Acid303SoloProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) {
    juce::ScopedNoDenormals noDenormals;
    const int samples = buffer.getNumSamples();
    buffer.clear();
    if (buffer.getNumChannels()<2) return;
    auto value = [this](const char* id) { return params.getRawParameterValue(id)->load(std::memory_order_relaxed); };
    acid303::Controls c;
    c.wave=value("wave"); c.shape=value("shape"); c.tune=value("tune");
    c.cutoff=value("cutoff"); c.mod=value("mod"); c.reso=value("reso"); c.lfoRate=value("lfoRate");
    c.envMod=value("envMod"); c.slideTime=value("slideTime"); c.decay=value("decay");
    c.accent=value("accent"); c.volume=value("volume"); c.masterVolume=value("masterVolume");
    c.compOn=value("compOn") >= 0.5f;
    c.compLow=value("compLow"); c.compMid=value("compMid"); c.compHigh=value("compHigh");
    c.compGain=value("compGain"); c.compWet=value("compWet");
    c.distOn=value("distOn") >= 0.5f; c.distWet=value("distWet");
    c.distDrive=value("distDrive"); c.distVolume=value("distVolume"); c.distEq=value("distEq");
    c.distType=juce::jlimit(0,1,static_cast<int>(value("distType")+0.5f));
    c.delayOn=value("delayOn") >= 0.5f; c.delayWet=value("delayWet");
    c.delayTime=value("delayTime"); c.delayFeedback=value("delayFeedback");
    c.delayMode=juce::jlimit(0,1,static_cast<int>(value("delayMode")+0.5f));
    c.delayWidth=value("delayWidth");
    c.reverbOn=value("reverbOn") >= 0.5f; c.reverbWet=value("reverbWet");
    c.reverbSize=value("reverbSize"); c.reverbVolume=value("reverbVolume");

    // The sequencer uses FL Studio's own PPQ transport and current BPM.
    // No standalone BPM parameter and no independent sequencer clock.
    bool foundBpm = false;
    bool hostPlaying = false;
    bool hasPpq = false;
    double ppqStart = 0.0;
    if (auto* playhead = getPlayHead()) {
        if (auto pos = playhead->getPosition()) {
            if (auto bpm = pos->getBpm()) {
                if (*bpm >= 1.0 && *bpm <= 999.0) {
                    c.bpm = static_cast<float>(*bpm);
                    foundBpm = true;
                }
            }
            hostPlaying = pos->getIsPlaying();
            if (auto ppq = pos->getPpqPosition()) {
                ppqStart = *ppq;
                hasPpq = true;
            }
        }
    }
    if (!foundBpm) c.bpm = hostBpm.load(std::memory_order_relaxed);
    hostBpm.store(c.bpm, std::memory_order_relaxed);
    hostTempoAvailable.store(foundBpm, std::memory_order_relaxed);

    const int mode = getPlayMode();
    const int seqSpeed = juce::jlimit(0, 3, static_cast<int>(value("seqRate") + 0.5f));
    midiGate.setTiming(1 << seqSpeed, getSequenceLength());
    if (mode != lastMode) {
        // Switching modes resets both synth and sequencer note states, without
        // double-triggering any lingering MIDI events from the old mode.
        stopSequence();
        engine.allNotesOff();
        haveLastPpqEnd = false;
        lastMode = mode;
    }

    // FL Studio moves the playhead when the user seeks, stops or loops. Clear
    // the trigger-note gate after large discontinuities: no sequence may keep
    // playing in a region where the original sustained MIDI note is absent.
    const bool jumped = mode == 1 && hostPlaying && hasPpq && haveLastPpqEnd
        && std::abs(ppqStart - lastPpqEnd) > 0.125;
    if (mode == 1 && (!hostPlaying || jumped)) {
        stopSequence();
        haveLastPpqEnd = false;
    }
    if (mode == 1 && hostPlaying && hasPpq) {
        lastPpqEnd = ppqStart + (static_cast<double>(samples) / sampleRateHz) * (c.bpm / 60.0);
        haveLastPpqEnd = true;
    } else {
        haveLastPpqEnd = false;
    }
    lastTransportRunning = mode == 1 && hostPlaying;

    auto cursor = midi.cbegin();
    const auto end = midi.cend();
    float* left = buffer.getWritePointer(0);
    float* right = buffer.getWritePointer(1);
    for (int i = 0; i < samples; ++i) {
        while (cursor != end && (*cursor).samplePosition <= i) {
            const auto m = (*cursor).getMessage();
            if (mode == 0) {
                if (m.isNoteOn()) engine.noteOn(m.getNoteNumber(), m.getFloatVelocity(), c);
                else if (m.isNoteOff()) engine.noteOff(m.getNoteNumber(), c);
                else if (m.isAllNotesOff() || m.isAllSoundOff()) engine.allNotesOff();
            } else if (hostPlaying) {
                // In sequencer mode, MIDI only opens/closes the playback gate.
                // The note's pitch does not transpose or overwrite programmed steps.
                if (m.isNoteOn()) midiGate.noteOn(m.getNoteNumber());
                else if (m.isNoteOff()) {
                    midiGate.noteOff(m.getNoteNumber());
                    if (!midiGate.active()) releaseSequenceVoice();
                } else if (m.isAllNotesOff() || m.isAllSoundOff()) {
                    stopSequence();
                }
            }
            ++cursor;
        }
        if (mode == 1 && hostPlaying) {
            // Sequence starts on MIDI note-on, even halfway through a DAW bar.
            // Step duration and loop length are host-automatable parameters.
            const int newStep = midiGate.stepToTrigger();
            if (newStep >= 0) {
                triggerSequenceStep(newStep, c);
                currentSequenceStep.store(newStep, std::memory_order_relaxed);
            }
            midiGate.advance(c.bpm, sampleRateHz);
        }
        const auto out = engine.render(c);
        left[i] = out.l;
        right[i] = out.r;
    }
}
void Acid303SoloProcessor::getStateInformation(juce::MemoryBlock& dest) {
    auto snapshot = params.copyState();
    juce::ValueTree patternNode("SEQUENCER");
    for (int i = 0; i < 16; ++i) {
        const juce::Identifier key("s" + juce::String(i));
        patternNode.setProperty(key,
                                static_cast<int>(steps[static_cast<size_t>(i)].load(std::memory_order_relaxed)),
                                nullptr);
    }
    snapshot.addChild(patternNode, -1, nullptr);
    if (auto xml = snapshot.createXml()) copyXmlToBinary(*xml, dest);
}
void Acid303SoloProcessor::setStateInformation(const void* data,int bytes) {
    if (auto xml=getXmlFromBinary(data,bytes)) {
        auto restored=juce::ValueTree::fromXml(*xml);
        if (restored.isValid() && restored.hasType("ACID303_SOLO")) {
            auto node = restored.getChildWithName("SEQUENCER");
            if (node.isValid()) {
                for (int i = 0; i < 16; ++i) {
                    const juce::Identifier key("s" + juce::String(i));
                    if (node.hasProperty(key))
                        steps[static_cast<size_t>(i)].store(static_cast<uint32_t>(
                            static_cast<int>(node.getProperty(key))), std::memory_order_relaxed);
                }
            }
            // SEQUENCER is non-parameter state. Remove it before passing the
            // restored APVTS state to JUCE: otherwise subsequent saves may
            // duplicate the child and reload stale notes from the first copy.
            if (node.isValid()) restored.removeChild(node, nullptr);
            params.replaceState(restored);
        }
    }
}

namespace {
juce::String presetExtension() { return ".acid303preset"; }
}
juce::File Acid303SoloProcessor::getBundlePresetFolder() {
    // Standard installation path recommended in the build instructions.
    // Protected by Windows (Program Files): mainly for supplied/factory presets.
   #if JUCE_WINDOWS
    auto dir = juce::File("C:\\Program Files\\Common Files\\VST3\\Acid303 Solo.vst3\\Contents\\Presets");
    return dir;
   #else
    return getUserPresetFolder();
   #endif
}
juce::File Acid303SoloProcessor::getUserPresetFolder() {
    // Resolve the REAL per-user Documents folder, including OneDrive redirection.
    // The folder is deliberately outside Program Files and the VST3 bundle so
    // uninstalling or updating the plugin never removes the user's presets.
    return juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
        .getChildFile("Acid303").getChildFile("presets");
}
juce::File Acid303SoloProcessor::getDefaultPresetFolder() {
    return getUserPresetFolder();
}
juce::Result Acid303SoloProcessor::preparePresetFolder() {
    const auto destination = getUserPresetFolder();
    if (auto result = destination.createDirectory(); result.failed())
        return juce::Result::fail("Impossible de creer le dossier : "
                                  + destination.getFullPathName() + "\n" + result.getErrorMessage());

    // One-time, non-destructive discovery of v0.20 presets. An existing
    // destination file is NEVER overwritten. Both the AppData fallback and
    // presets saved inside the old VST3 are supported.
    const std::array<juce::File, 2> legacy = {{
        juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
            .getChildFile("Acid303 Solo").getChildFile("Presets"),
        getBundlePresetFolder()
    }};
    for (const auto& oldFolder : legacy) {
        if (!oldFolder.isDirectory() || oldFolder == destination) continue;
        for (const auto& oldFile : oldFolder.findChildFiles(juce::File::findFiles, false, "*.acid303preset")) {
            const auto target = destination.getChildFile(oldFile.getFileName());
            if (!target.exists()) {
                // Leave the original untouched. If Windows denies access to an
                // old location, the user can still import manually with LOAD.
                oldFile.copyFileTo(target);
            }
        }
    }
    return juce::Result::ok();
}
juce::String Acid303SoloProcessor::getCurrentPresetName() const {
    const auto name = params.state.getProperty(juce::Identifier("presetDisplayName"), juce::var("INIT PATCH")).toString().trim();
    return name.isNotEmpty() ? name : juce::String("INIT PATCH");
}
void Acid303SoloProcessor::setCurrentPresetName(const juce::String& name) {
    const auto cleaned = name.trim().substring(0, 70);
    params.state.setProperty(juce::Identifier("presetDisplayName"),
        cleaned.isNotEmpty() ? cleaned : juce::String("INIT PATCH"), nullptr);
    updateHostDisplay(juce::AudioProcessor::ChangeDetails{}.withNonParameterStateChanged(true));
}
juce::Result Acid303SoloProcessor::savePresetToFile(const juce::File& target) {
    const auto file = target.hasFileExtension(presetExtension())
        ? target : target.withFileExtension(presetExtension());
    if (auto result = file.getParentDirectory().createDirectory(); result.failed()) return result;
    setCurrentPresetName(file.getFileNameWithoutExtension());
    juce::MemoryBlock stateData;
    getStateInformation(stateData);
    if (stateData.isEmpty()) return juce::Result::fail("Le preset est vide.");
    // Atomic replacement: a failed write should not destroy a valid existing preset.
    juce::TemporaryFile temp(file);
    if (!temp.getFile().replaceWithData(stateData.getData(), stateData.getSize()))
        return juce::Result::fail("Ecriture impossible : dossier protege ou espace disque insuffisant.");
    if (!temp.overwriteTargetFileWithTemporary())
        return juce::Result::fail("Impossible de finaliser le preset.");
    return juce::Result::ok();
}
juce::Result Acid303SoloProcessor::loadPresetFromFile(const juce::File& file) {
    if (!file.existsAsFile()) return juce::Result::fail("Preset introuvable.");
    if (!file.hasFileExtension(presetExtension()))
        return juce::Result::fail("Extension invalide : utiliser .acid303preset");
    juce::MemoryBlock bytes;
    if (!file.loadFileAsData(bytes)) return juce::Result::fail("Lecture du preset impossible.");
    auto xml = getXmlFromBinary(bytes.getData(), static_cast<int>(bytes.getSize()));
    if (xml == nullptr || !xml->hasTagName("ACID303_SOLO"))
        return juce::Result::fail("Ce fichier n'est pas un preset Acid303 valide.");
    setStateInformation(bytes.getData(), static_cast<int>(bytes.getSize()));
    setCurrentPresetName(file.getFileNameWithoutExtension());
    updateHostDisplay(juce::AudioProcessor::ChangeDetails{}.withNonParameterStateChanged(true));
    return juce::Result::ok();
}

juce::File Acid303SoloProcessor::getUserMidiFolder() {
    return juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
        .getChildFile("Acid303").getChildFile("midi");
}

juce::Result Acid303SoloProcessor::prepareMidiFolder() {
    const auto folder = getUserMidiFolder();
    if (folder.isDirectory()) return juce::Result::ok();
    return folder.createDirectory();
}

juce::File Acid303SoloProcessor::getUserSkinFolder() {
    return juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
        .getChildFile("Acid303").getChildFile("skins");
}

juce::Result Acid303SoloProcessor::prepareSkinFolder() {
    const auto folder = getUserSkinFolder();
    if (folder.isDirectory()) return juce::Result::ok();
    return folder.createDirectory();
}

juce::Result Acid303SoloProcessor::exportMidiToFile(const juce::File& requested) const {
    const auto file = requested.hasFileExtension(".mid") ? requested : requested.withFileExtension(".mid");
    acid303::midi::Pattern pattern;
    pattern.length = getSequenceLength();
    pattern.rate = juce::jlimit(0, 3, static_cast<int>(params.getRawParameterValue("seqRate")->load() + 0.5f));
    for (int i = 0; i < 16; ++i) pattern.steps[static_cast<size_t>(i)] = getStep(i);
    const auto bytes = acid303::midi::encode(pattern);
    if (auto result = file.getParentDirectory().createDirectory(); result.failed())
        return result;
    // Safe replacement: unlike FileOutputStream, this does not append to an
    // existing .mid file when overwriting a previously exported pattern.
    if (!file.replaceWithData(bytes.data(), bytes.size()))
        return juce::Result::fail("Ecriture MIDI impossible : " + file.getFullPathName());
    return juce::Result::ok();
}

juce::Result Acid303SoloProcessor::importMidiFromFile(const juce::File& file) {
    if (!file.existsAsFile()) return juce::Result::fail("Fichier MIDI introuvable.");
    if (!file.hasFileExtension(".mid;.midi"))
        return juce::Result::fail("Format invalide. Utiliser un fichier MIDI .mid ou .midi.");
    if (file.getSize() > 8 * 1024 * 1024)
        return juce::Result::fail("Le fichier MIDI est trop volumineux (maximum 8 Mo).");
    juce::MemoryBlock block;
    if (!file.loadFileAsData(block)) return juce::Result::fail("Impossible de lire le fichier MIDI.");
    if (block.getSize() == 0) return juce::Result::fail("Le fichier MIDI est vide.");
    const auto* start = static_cast<const uint8_t*>(block.getData());
    std::vector<uint8_t> bytes(start, start + block.getSize());
    const int currentRate = juce::jlimit(0,3, static_cast<int>(params.getRawParameterValue("seqRate")->load() + 0.5f));
    auto result = acid303::midi::decode(bytes, currentRate);
    if (!result.ok) return juce::Result::fail(juce::String(result.error));
    // Update all sixteen pads, including disabled/unused ones when native
    // metadata is present. Audio thread reads these values atomically.
    for (int i = 0; i < 16; ++i)
        steps[static_cast<size_t>(i)].store(acid303::seq::pack(result.pattern.steps[static_cast<size_t>(i)]),
                                           std::memory_order_relaxed);
    const auto setHostParameter = [this](const char* id, int v) {
        if (auto* p = params.getParameter(id)) {
            p->beginChangeGesture();
            p->setValueNotifyingHost(p->convertTo0to1(static_cast<float>(v)));
            p->endChangeGesture();
        }
    };
    setHostParameter("seqRate", result.pattern.rate);
    setHostParameter("seqLength", result.pattern.length);
    updateHostDisplay(juce::AudioProcessor::ChangeDetails{}.withNonParameterStateChanged(true));
    return juce::Result::ok();
}

juce::AudioProcessorEditor* Acid303SoloProcessor::createEditor() { return new Acid303SoloEditor(*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new Acid303SoloProcessor(); }
