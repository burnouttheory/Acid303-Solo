Acid303 Solo v3.2.0
Burnout Theory — Free acid bass synthesizer for Windows 64-bit (VST3)
Acid303 Solo is a monophonic acid bass synthesizer developed with JUCE and designed for music production in FL Studio. It combines a 16-step sequencer, classic acid-style sound shaping and modern wavetable and Unison controls.
Download the latest release
What's new in v3.2.0
8 selectable waveforms: Saw, Pulse, Triangle, Sine, Square, Hollow, Formant and Folded.
WAVE LEFT / WAVE RIGHT selection, MORPH blending and a waveform visualizer.
Extended SHAPE range for more varied timbres.
UNISON: up to 8 oscillators per note, with DETUNE and BLEND (monophonic MIDI behavior is preserved).
FL Studio VST3 automation for Morph, Detune, Blend, the four effect ON/OFF switches, Pattern Group and Pattern.
Improved W&B and TB303 GREY skin readability.
Smoother note onset to reduce clicks.
154 factory presets with 64 patterns each — 9,856 patterns in total.
Main features
16-step sequencer with notes, accents, slides, adjustable loop length and randomization.
Eight pattern groups (1A, 1B, 2A, 2B, 3A, 3B, 4A, 4B), with eight patterns per group.
Pattern triggering from FL Studio's Piano Roll using MIDI notes 36–99.
Global transpose (±12 semitones) and MIDI import/export.
Filter, envelope, 3-band compressor, Tube/Bitcrusher distortion, stereo delay and reverb.
French and English interfaces, selectable skins and preset saving/loading.
Installation
Close FL Studio and back up important `.flp` projects and personal `.acid303preset` files.
Download Acid303Solo_Setup_3.2.0_OFFICIEL.exe from GitHub Releases.
Run the installer, then reopen FL Studio. Rescan installed VST3 plugins if needed.
Insert Acid303 Solo and use LOAD PRESET to explore the factory collection.
The installer keeps the official VST3 identity used in v3.0.0. Custom presets are intended to remain untouched; older factory preset folders may also remain available.
Using the 64 patterns in FL Studio
Switch to SEQUENCER mode and enable MIDI BANKS. In the Piano Roll, MIDI note 36 selects pattern 1A/1, note 37 selects 1A/2, note 44 selects 1B/1, and note 99 selects 4B/8. MIDI trigger notes select patterns, not bass pitches.
After editing a pattern or sound, use SAVE PRESET to save the reusable `.acid303preset` file; save your FL Studio project separately.
Documentation
The release includes the English and French PDF manuals, which contain a v3.2.0 addendum and the complete earlier V3 reference manual.
Building from source (Windows)
Requirements: Visual Studio 2022 C++ tools, CMake 3.22+, Git and JUCE 8.0.10 (fetched by the project or provided locally). Inno Setup 6 is optional for packaging.
Extract the source ZIP to a short Windows path and run BUILD_WINDOWS.bat. The project includes CMakeLists.txt and the Inno Setup script in Installeur/Acid303Solo.iss. Keep the original installer AppId `Acid303SoloVST3` when building an upgrade.
License and support
Read LICENSE and LICENSES/ in the source package for project and third-party licensing conditions, including JUCE. Source availability does not replace the applicable licenses.
Developer: Burnout Theory (Belgium)  
Project: https://github.com/burnouttheory/Acid303-Solo
