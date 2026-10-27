#pragma once
// Acid303 Solo: original DSP. JUCE-independent so synthesis/effects can be tested on any platform.
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>

namespace acid303 {
constexpr float pi = 3.14159265358979323846f;
inline float clampf(float x, float lo, float hi) { return std::clamp(x, lo, hi); }
inline float hzFromMidi(float note) { return 440.0f * std::pow(2.0f, (note - 69.0f) / 12.0f); }
inline float polyBlep(float t, float dt) {
    if (t < dt) { const float x = t / dt; return x + x - x * x - 1.0f; }
    if (t > 1.0f - dt) { const float x = (t - 1.0f) / dt; return x * x + x + x + 1.0f; }
    return 0.0f;
}

struct Controls {
    float wave = 0.0f;          // 0 saw -> 1 pulse
    float shape = 0.50f;        // pulse width / saw bend
    float tune = 0.0f;          // -12..+12 semitones
    float cutoff = 0.38f;      // lowpass
    float mod = 0.0f;          // LFO cutoff depth
    float reso = 0.72f;
    float lfoRate = 1.0f;       // Hz
    float envMod = 0.70f;      // filter envelope amount
    float slideTime = 0.11f;   // legato slide time in seconds
    float decay = 0.36f;       // note/filter decay normalised
    float accent = 0.68f;      // strength of velocity accent
    float volume = 0.70f;
    float masterVolume = 0.85f; // independent master gain AFTER all effects
    // Independent pre-distortion 3-band dynamics compressor.
    bool compOn = false;
    float compLow = 0.48f, compMid = 0.48f, compHigh = 0.48f;
    float compGain = 1.0f, compWet = 0.75f;
    bool distOn = false;        // true = effect active; OFF = dry passthrough
    float distWet = 0.72f;      // dry/wet crossfade 0..1
    float distDrive = 0.58f;
    float distVolume = 0.85f;
    float distEq = 0.5f;       // dark <0.5 / bright >0.5
    int distType = 0;           // Original preset mapping: 0 = Tube (new pedal-inspired sound), 1 = Bitcrusher
    bool delayOn = false;       // OFF = no delay, no tail
    float delayWet = 0.30f;
    float delayTime = 0.375f;  // synced subdivision, 0..1
    float delayFeedback = 0.22f;
    int delayMode = 0;           // 0: NORMAL, 1: PING-PONG
    float delayWidth = 0.55f;    // 0: centred/mono echo, 1: maximum stereo spread
    bool reverbOn = false;      // OFF = no reverb, no tail
    float reverbWet = 0.40f;
    float reverbSize = 0.42f;
    float reverbVolume = 0.45f;
    float bpm = 120.0f;        // ALWAYS supplied by DAW when host reports tempo
};
// V2.0.5 - Cutoff spans the audible range logarithmically.  The old DSP
// added a fixed several-kHz envelope offset, making a nearly closed knob
// sound open.  This proportional octave sweep preserves acid accents while
// allowing a genuinely dark minimum position and fully open maximum.
// Kept pure and JUCE-independent for reproducible DSP regression tests.
inline float cutoffFrequencyHz(const Controls& c, float filterEnv,
                               float noteVelocity, float lfo, float sampleRate) {
    const float position = clampf(c.cutoff, 0.0f, 1.0f);
    const float baseHz = 45.0f * std::pow(16000.0f / 45.0f,
                                         std::pow(position, 0.68f));
    const float sweepOctaves = clampf(filterEnv, 0.0f, 1.7f)
        * clampf(c.envMod, 0.0f, 1.0f)
        * (1.60f + 0.65f * clampf(noteVelocity, 0.0f, 1.0f)
                     * clampf(c.accent, 0.0f, 1.0f))
        * (0.35f + 0.65f * position);
    const float lfoOctaves = lfo * clampf(c.mod, 0.0f, 1.0f) * 2.4f;
    return clampf(baseHz * std::exp2(sweepOctaves + lfoOctaves),
                  30.0f, sampleRate * 0.42f);
}

struct Stereo { float l = 0.0f, r = 0.0f; };

struct DelayLine {
    std::vector<float> data;
    std::vector<uint32_t> generations; // virtual clearing avoids blocking the audio callback
    uint32_t generation = 1;
    int pos = 0;
    void prepare(int size) {
        data.assign(static_cast<size_t>(std::max(size,2)), 0.0f);
        generations.assign(data.size(), 0u);
        generation = 1;
        pos = 0;
    }
    void reset() {
        ++generation;
        if (generation == 0u) { // very rare wraparound
            std::fill(generations.begin(), generations.end(), 0u);
            generation = 1;
        }
        pos = 0;
    }
    float read(int delaySamples) const {
        if (data.empty()) return 0.0f;
        const int size = static_cast<int>(data.size());
        const int n = std::clamp(delaySamples, 1, size-1);
        int index = pos - n;
        if (index < 0) index += size;
        const auto at = static_cast<size_t>(index);
        return generations[at] == generation ? data[at] : 0.0f;
    }
    float process(float x, int delaySamples, float feedback) {
        const float delayed = read(delaySamples);
        write(x + delayed * feedback);
        return delayed;
    }
    void write(float x) {
        if (data.empty()) return;
        const auto at = static_cast<size_t>(pos);
        data[at] = clampf(x, -8.0f, 8.0f);
        generations[at] = generation;
        if (++pos >= static_cast<int>(data.size())) pos = 0;
    }
};

class Engine {
public:
    void prepare(double rate) {
        sr = clampf(static_cast<float>(rate), 8000.0f, 192000.0f);
        delayL.prepare(static_cast<int>(sr * 12.1f) + 8);
        delayR.prepare(static_cast<int>(sr * 12.1f) + 8);
        for (int ch = 0; ch < 2; ++ch) {
            for (int i = 0; i < 4; ++i) {
                const float base[4] = {0.0297f, 0.0371f, 0.0411f, 0.0437f};
                comb[ch][i].prepare(static_cast<int>(sr * (base[i] + ch * 0.0012f)));
            }
            for (int i = 0; i < 2; ++i) {
                const float base[2] = {0.005f, 0.0017f};
                allpass[ch][i].prepare(static_cast<int>(sr * (base[i] + ch * 0.0003f)));
            }
        }
        reset();
    }
    void reset() {
        phase = 0.0f; lfoPhase = 0.0f;
        frequency = targetFrequency = hzFromMidi(48.0f);
        envelope = filterEnvelope = velocity = 0.0f;
        filter1 = filter2 = 0.0f; smoothedCutoffHz = 0.0f; toneLP_L = toneLP_R = 0.0f;
        crusherSample = 0.0f; crusherCount = 0; lastDistType = -1;
        acidInputPrev = acidLow = acidPostLp1 = acidPostLp2 = acidDcIn = acidDcOut = 0.0f;
        compLowLP = compHighLP = 0.0f;
        compEnvelope.fill(0.0f);
        currentNote = -1; gate = false; releasing = false;
        heldNotes.fill(false);
        delayL.reset(); delayR.reset();
        previousDelayOn = false; previousReverbOn = false;
        previousDelayMode = -1;
        for (auto& channel : comb) for (auto& line : channel) line.reset();
        for (auto& channel : allpass) for (auto& line : channel) line.reset();
    }
    void noteOn(int note, float vel, const Controls& c) {
        note = std::clamp(note,0,127);
        vel = clampf(vel, 0.0f, 1.0f);
        const bool legato = gate && currentNote >= 0;
        heldNotes[static_cast<size_t>(note)] = true;
        currentNote = note; gate = true; releasing = false;
        targetFrequency = hzFromMidi(static_cast<float>(note) + c.tune);
        // Overlapping piano-roll notes create legato/slide; the first note starts immediately.
        if (!legato || envelope < 0.001f) frequency = targetFrequency;
        velocity = vel;
        envelope = clampf((0.60f + 0.40f * vel) * (1.0f + vel * c.accent * 0.40f), 0.0f, 1.7f);
        filterEnvelope = 0.7f + 0.85f * vel * c.accent;
    }
    void noteOff(int note, const Controls& c) {
        (void)c;
        note = std::clamp(note,0,127);
        heldNotes[static_cast<size_t>(note)] = false;
        if (currentNote != note) return;
        // Note priority for overlapping notes: resume the most recently available held pitch.
        int alternate = -1;
        for (int i = 127; i >= 0; --i)
            if (heldNotes[static_cast<size_t>(i)]) { alternate = i; break; }
        if (alternate >= 0) {
            currentNote = alternate;
            targetFrequency = hzFromMidi(static_cast<float>(alternate) + c.tune);
        } else {
            currentNote = -1; gate = false; releasing = true;
        }
    }
    void allNotesOff() { heldNotes.fill(false); currentNote=-1; gate=false; releasing=true; }

    // Tempo synchronisation: delay subdivisions of a beat (quarter note).
    // Time dial selects subdivisions 1/16,1/8,3/16,1/4,3/8,1/2,3/4,1/1,2/1 beats.
    static float syncedBeats(float time) {
        static constexpr float beats[] = {0.25f,0.5f,0.75f,1.0f,1.5f,2.0f,3.0f,4.0f,8.0f};
        const int index = std::clamp(static_cast<int>(time * 8.999f), 0, 8);
        return beats[index];
    }
    int delaySamples(const Controls& c) const {
        const float tempo = clampf(c.bpm, 40.0f, 400.0f);
        return std::clamp(static_cast<int>(sr * (60.0f / tempo) * syncedBeats(c.delayTime)),
                          1, static_cast<int>(delayL.data.size()) - 1);
    }

    // ACID DRIVE: not a Roland/BOSS/Ibanez schematic replica. Inspired by the
    // frequency-selective drive topology common to classic guitar pedals.
    // Pre-emphasis leaves some fundamental intact while driving upper mids;
    // asymmetric diode-like waveshaping adds odd/even harmonics; a high-drive
    // second clipping stage gives a more urgent acid-techno character.
    // Oversampled nonlinear section (4x, linear input interpolation) with
    // 2-pole output lowpass; intentionally avoids allocations on audio thread.
    // DC blocker counters asymmetrical offset. Parameter changes are safe.
    float processAcidDrive(float in, float drive) {
        constexpr int factor = 4;
        const float osrate = sr * factor;
        const float hpA = 1.0f - std::exp(-2.0f*pi*620.0f/osrate);
        const float postA = 1.0f - std::exp(-2.0f*pi*8500.0f/osrate);
        const float gain = 1.0f + 116.0f*drive*drive;
        const float stack = 0.78f*drive*drive;
        float value = 0.0f;
        for (int k = 0; k < factor; ++k) {
            const float sub = acidInputPrev + (in-acidInputPrev)*((k+1)*0.25f);
            acidLow += hpA*(sub-acidLow);
            // Retain ~42% lows, distort upper mids more strongly.
            const float focussed = 0.42f*sub + 0.84f*(sub-acidLow);
            const float x = focussed * gain;
            // Unlike Tube, this is not simply a tanh curve: distinct unequal
            // positive/negative diode characteristics plus a stackable harder knee.
            const float diode = x>=0.0f ? 0.68f*std::tanh(x/0.68f)
                                          : 0.88f*std::tanh(x/0.88f);
            const float harder = std::clamp(diode * (1.0f+2.1f*drive),-0.76f,0.61f);
            const float clipped = diode*(1.0f-stack)+harder*stack;
            // Suppress most out-of-band alias energy before downsampling.
            acidPostLp1 += postA*(clipped-acidPostLp1);
            acidPostLp2 += postA*(acidPostLp1-acidPostLp2);
            value += acidPostLp2*0.25f;
        }
        acidInputPrev = in;
        const float dcR = std::exp(-2.0f*pi*18.0f/sr);
        const float out = value-acidDcIn + dcR*acidDcOut;
        acidDcIn = value;
        acidDcOut = out;
        return clampf(out, -1.0f, 1.0f);
    }

    Stereo render(const Controls& c) {
        float source = 0.0f;
        const float rate = 1.0f / sr;
        lfoPhase += c.lfoRate * rate;
        if (lfoPhase >= 1.0f) lfoPhase -= std::floor(lfoPhase);
        if (envelope > 0.00001f) {
            const float decSeconds = 0.09f + c.decay * 1.5f;
            envelope *= std::exp(-rate / (releasing ? 0.065f : decSeconds));
            filterEnvelope *= std::exp(-rate / (0.06f + c.decay * 0.55f));
            if (currentNote >= 0) targetFrequency = hzFromMidi(float(currentNote) + c.tune);
            // Time constant for pitch glide (legato only); no glide for fresh notes.
            const float glideFactor = 1.0f - std::exp(-rate / std::max(0.002f, c.slideTime));
            frequency += (targetFrequency - frequency) * glideFactor;
            const float step = clampf(frequency * rate, 0.000001f, 0.40f);
            phase += step;
            if (phase >= 1.0f) phase -= 1.0f;

            const float pivot = 0.18f + 0.64f * c.shape;
            const float warped = phase < pivot ? phase * (0.5f/pivot)
                : 0.5f + (phase-pivot) * (0.5f/(1.0f-pivot));
            const float saw = 2.0f * warped - 1.0f - polyBlep(phase,step);
            const float duty = 0.15f + 0.70f * c.shape;
            const float shifted = std::fmod(phase - duty + 1.0f, 1.0f);
            const float pulse = (phase < duty ? 1.0f : -1.0f)
                                + polyBlep(phase,step) - polyBlep(shifted,step);
            source = saw * (1.0f - c.wave) + pulse * c.wave;

            const float lfo = std::sin(2.0f*pi*lfoPhase);
            // V2.0.5: much wider, perceptually distributed cutoff movement.
            // 4 ms frequency smoothing prevents clicks during FL Studio automation.
            const float desiredCutoffHz = cutoffFrequencyHz(
                c, filterEnvelope, velocity, lfo, sr);
            if (smoothedCutoffHz <= 0.0f) smoothedCutoffHz = desiredCutoffHz;
            const float cutoffSmoothing = 1.0f - std::exp(-rate / 0.004f);
            smoothedCutoffHz += (desiredCutoffHz - smoothedCutoffHz) * cutoffSmoothing;
            const float g = std::tan(pi * smoothedCutoffHz / sr);
            const float q = 0.72f + c.reso * 11.5f;
            const float k = 1.0f / q;
            const float a1 = 1.0f / (1.0f + g*(g+k));
            const float a2 = g*a1;
            const float a3 = g*a2;
            // Soft nonlinear input adds harmonics around resonant sweeps, while
            // the stable TPT filter below preserves bounded behaviour.
            const float filteredInput = std::tanh(source * (1.05f + c.reso * 0.70f));
            const float v3 = filteredInput - filter2;
            const float v1 = a1*filter1 + a2*v3;
            const float v2 = filter2 + a2*filter1 + a3*v3;
            filter1 = 2.0f*v1 - filter1;
            filter2 = 2.0f*v2 - filter2;
            source = std::tanh(v2 * (1.8f + c.reso * 2.7f))
                     * envelope * c.volume * 0.37f;
        }

        // Multiband dynamics BEFORE distortion: Low <220Hz, Mid 220-2800Hz,
        // High >2800Hz. Each dial sets its band's compression intensity.
        // One-pole crossovers sum back to original sample on dry path.
        if (c.compOn) {
            const float aLo = 1.0f - std::exp(-2.0f*pi*220.0f*rate);
            const float aHi = 1.0f - std::exp(-2.0f*pi*2800.0f*rate);
            compLowLP += aLo*(source-compLowLP);
            compHighLP += aHi*(source-compHighLP);
            const float low = compLowLP;
            const float mid = compHighLP - compLowLP;
            const float high = source - compHighLP;
            const float bands[3] = {low, mid, high};
            const float amounts[3] = {c.compLow,c.compMid,c.compHigh};
            float processed = 0.0f;
            const float attack = 1.0f-std::exp(-rate/0.006f);
            const float release = 1.0f-std::exp(-rate/0.115f);
            for (int b=0; b<3; ++b) {
                const float level = std::abs(bands[b]);
                compEnvelope[static_cast<size_t>(b)] +=
                    (level > compEnvelope[static_cast<size_t>(b)] ? attack : release)
                    * (level-compEnvelope[static_cast<size_t>(b)]);
                const float amount = clampf(amounts[b],0.0f,1.0f);
                const float threshold = 0.12f-amount*0.095f;
                const float ratio = 1.0f+amount*12.0f;
                const float env = compEnvelope[static_cast<size_t>(b)];
                const float reduction = env > threshold
                    ? std::pow(threshold/env,1.0f-1.0f/ratio) : 1.0f;
                processed += bands[b]*reduction;
            }
            const float mix = clampf(c.compWet,0.0f,1.0f);
            source = source*(1.0f-mix)+processed*clampf(c.compGain,0.0f,2.0f)*mix;
        } else {
            // Do not allow old envelopes to shape audio upon reactivation.
            compLowLP = compHighLP = source;
            compEnvelope.fill(0.0f);
        }

        // Two unchanged preset identities: 0=TUBE, 1=BITCRUSHER.
        // TUBE now uses an acid-oriented, external-pedal-inspired asymmetrical
        // overdrive. Older Tube presets load unchanged but WILL sound different.
        // The vintage TB-303 has no integrated distortion.
        float l = source, r = source;
        if (c.distOn) {
            const int type = std::clamp(c.distType, 0, 1);
            const float drive = clampf(c.distDrive, 0.0f, 1.0f);
            const float preGain = 1.0f + 54.0f * drive * drive;
            const float x = source * preGain;
            if (type != lastDistType) {
                crusherCount = 0;
                crusherSample = 0.0f;
                if (type == 0) {
                    acidInputPrev = acidLow = acidPostLp1 = acidPostLp2 = acidDcIn = acidDcOut = 0.0f;
                }
                lastDistType = type;
            }
            float driven = 0.0f;
            switch (type) {
                case 0: { // TUBE name retained; replacement: 303-focused diode overdrive
                    driven = processAcidDrive(source, drive);
                    break;
                }
                case 1: { // Bitcrusher: drive adjusts quantisation and sample rate reduction
                    const int bits = std::clamp(14 - static_cast<int>(drive * 11.0f), 3, 14);
                    const int levels = (1 << (bits-1)) - 1;
                    const int holdSamples = std::max(1, static_cast<int>(
                        (1.0f + drive * drive * 43.0f) * sr / 44100.0f));
                    if (crusherCount <= 0) {
                        const float in = clampf(source * (1.0f + drive * 8.0f), -1.0f, 1.0f);
                        crusherSample = std::round(in * float(levels)) / float(levels);
                        crusherCount = holdSamples;
                    }
                    --crusherCount;
                    driven = crusherSample;
                    break;
                }
            }

            // Distortion EQ applies to both modes.
            const float eqCut = 350.0f + c.distEq * 14000.0f;
            const float eqA = 1.0f - std::exp(-2.0f * pi * eqCut * rate);
            toneLP_L += eqA * (driven - toneLP_L);
            toneLP_R += eqA * (driven - toneLP_R);
            const float dark = clampf((0.5f - c.distEq) * 2.0f, 0.0f, 1.0f);
            const float bright = clampf((c.distEq - 0.5f) * 2.0f, 0.0f, 1.0f);
            const float high = driven - toneLP_L;
            const float toned = driven * (1.0f - 0.60f * dark)
                              + toneLP_L * (0.60f * dark)
                              + high * (bright * 0.75f);
            const float mix = clampf(c.distWet, 0.0f, 1.0f);
            const float wet = toned * clampf(c.distVolume, 0.0f, 1.0f) * 1.25f;
            l = source * (1.0f - mix) + wet * mix;
            r = l;
        } else {
            toneLP_L = source; toneLP_R = source;
            lastDistType = -1; // restart nonlinear mode safely on next enable
        }

        // DAW-synchronised dual mode stereo delay. Original DRY/WET behaviour is
        // preserved: 0% = dry, 100% = echoes only. All operations are realtime-safe.
        const int selectedDelayMode = std::clamp(c.delayMode, 0, 1);
        // Reset when disabled or changing routing to avoid orphaned tails.
        if ((!c.delayOn && previousDelayOn)
            || (c.delayOn && previousDelayMode >= 0 && selectedDelayMode != previousDelayMode)) {
            delayL.reset();
            delayR.reset();
        }
        previousDelayOn = c.delayOn;
        previousDelayMode = c.delayOn ? selectedDelayMode : -1;
        if (c.delayOn) {
            const int delayN = delaySamples(c);
            const float feedback = clampf(c.delayFeedback, 0.0f, 0.89f);
            const float width = clampf(c.delayWidth, 0.0f, 1.0f);
            const float dryL = l, dryR = r;
            float echoL = 0.0f, echoR = 0.0f;
            if (selectedDelayMode == 0) {
                // NORMAL: independent repetitions, slightly offset on the right
                // at wider settings for true stereo spread even from a mono 303.
                const int rightN = std::min(delayN + static_cast<int>(sr * 0.020f * width),
                                            static_cast<int>(delayR.data.size()) - 1);
                const float tapL = delayL.read(delayN);
                const float tapR = delayR.read(rightN);
                delayL.write(dryL + tapL * feedback);
                delayR.write(dryR + tapR * feedback);
                const float centre = (tapL + tapR) * 0.5f;
                echoL = centre * (1.0f - width) + tapL * width;
                echoR = centre * (1.0f - width) + tapR * width;
            } else {
                // PING-PONG: first repeat on LEFT, then RIGHT, alternating with
                // each full synced delay interval. WIDTH determines how far
                // apart the echoes are, without changing the rhythm.
                const float mono = (dryL + dryR) * 0.5f;
                const float tapL = delayL.read(delayN);
                const float tapR = delayR.read(delayN);
                delayL.write(mono + tapR * feedback);
                delayR.write(tapL * feedback);
                const float centre = (tapL + tapR) * 0.5f;
                echoL = centre * (1.0f - width) + tapL * width;
                echoR = centre * (1.0f - width) + tapR * width;
            }
            const float mix = clampf(c.delayWet, 0.0f, 1.0f);
            l = dryL * (1.0f - mix) + echoL * mix;
            r = dryR * (1.0f - mix) + echoR * mix;
        }

        // Stereo Schroeder reverb: SIZE controls tail length, VOLUME its wet output.
        // DRY/WET controls the original/effected proportion; OFF = clean bypass.
        if (!c.reverbOn && previousReverbOn) {
            for (auto& channel : comb) for (auto& line : channel) line.reset();
            for (auto& channel : allpass) for (auto& line : channel) line.reset();
        }
        previousReverbOn = c.reverbOn;
        if (c.reverbOn) {
            const float size = clampf(c.reverbSize,0.0f,1.0f);
            const float mono = 0.5f*(l+r);
            const float feedbackRoom = 0.60f + size*0.32f;
            float accum[2] = {0.0f,0.0f};
            for (int ch=0; ch<2; ++ch) {
                for (auto& line : comb[ch]) {
                    const float previous = line.read(static_cast<int>(line.data.size())-1);
                    line.write(mono*0.26f + previous*feedbackRoom);
                    accum[ch] += previous*0.25f;
                }
                for (auto& line : allpass[ch]) {
                    const float stored = line.read(static_cast<int>(line.data.size())-1);
                    const float next = -0.5f*accum[ch] + stored;
                    line.write(accum[ch]+0.5f*stored);
                    accum[ch] = next;
                }
            }
            const float mix = clampf(c.reverbWet,0.0f,1.0f);
            const float vol = clampf(c.reverbVolume,0.0f,1.0f);
            l = l*(1.0f-mix) + accum[0]*1.25f*vol*mix;
            r = r*(1.0f-mix) + accum[1]*1.25f*vol*mix;
        }
        // Soft limiter avoids accidental digital overload when feedback/resonance is high.
        // Separate, automation-ready master volume, downstream of ALL FX.
        const float master = clampf(c.masterVolume, 0.0f, 1.0f);
        return {std::tanh(l) * master, std::tanh(r) * master};
    }
private:
    float sr = 44100.0f;
    float phase = 0.0f, lfoPhase = 0.0f;
    float frequency = 130.81278f, targetFrequency = 130.81278f;
    float envelope = 0.0f, filterEnvelope = 0.0f, velocity = 0.0f;
    float filter1 = 0.0f, filter2 = 0.0f;
    float smoothedCutoffHz = 0.0f;
    float toneLP_L = 0.0f, toneLP_R = 0.0f;
    float acidInputPrev = 0.0f, acidLow = 0.0f;
    float acidPostLp1 = 0.0f, acidPostLp2 = 0.0f;
    float acidDcIn = 0.0f, acidDcOut = 0.0f;
    float compLowLP = 0.0f, compHighLP = 0.0f;
    std::array<float,3> compEnvelope{};
    float crusherSample = 0.0f;
    int crusherCount = 0, lastDistType = -1;
    bool previousDelayOn = false, previousReverbOn = false;
    int previousDelayMode = -1;
    int currentNote = -1;
    bool gate = false, releasing = false;
    std::array<bool,128> heldNotes{};
    DelayLine delayL, delayR;
    std::array<std::array<DelayLine,4>,2> comb;
    std::array<std::array<DelayLine,2>,2> allpass;
};
} // namespace acid303
