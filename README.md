# Acid303 Solo v3.0.0 — Burnout Theory

Official source project for Acid303 Solo v3.0.0 (Windows VST3, JUCE 8.0.10).

## Build on Windows

1. Install Visual Studio 2022 C++ Desktop tools, CMake, Git, and Inno Setup 6.
2. Unpack this source ZIP into a short local directory (for example `C:\Acid303\V3Official`).
3. Run `COMPILER_ET_INSTALLATEUR.cmd`.
4. Find `Installeur\Output\Acid303Solo_Setup_3.0.0_OFFICIEL.exe`.
5. Check the installer and `Documents\Acid303` backup before distribution.

## Upgrade and compatibility

V3 uses the **same VST3 identity** as v2.0.6 (`Ac03`/`A303`, `info.acid303.solo`, `Acid303 Solo.vst3`, Inno `AppId=Acid303SoloVST3`). Existing user presets and FL Studio project state are retained subject to DAW version and environment. A **separately developed V3 TEST** using another identity remains a distinct legacy plugin; do not delete it while old projects still refer to it.

FactoryV3 distributes 154 factory presets with all 64 patterns populated. The runtime migrates known Factory preset paths, backs up older factory files before replacing them, and does not overwrite Factory files already containing 64-pattern V3 data. Back up any customized old Factory presets before upgrading. Personal User presets are preserved.

## Main V3 features

- 64 selectable patterns per preset: 8 groups × 8 slots.
- MIDI trigger map notes 36–99, MIDI MAP 64, live bank-selector synchronization.
- Non-destructive global transpose ±12 semitones.
- 154 factory presets (9,856 patterns), 12 skins including TB303 GREY neon green, 174 MIDI resources including the mapping demo.
- Full French and English PDF guides in `Manuals`.

**SAVE PRESET after editing a pattern**, including RANDOM 16, to save your changes in the preset file.

## License

Uses JUCE 8.0.10 under the license applicable to the developer/distributor. Read the `LICENSES` directory and verify current commercial eligibility before distribution. All required upstream notices must accompany binary releases.

Official releases: https://github.com/burnouttheory/Acid303-Solo/releases
