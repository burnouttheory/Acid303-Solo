cmake_minimum_required(VERSION 3.22)
project(Acid303Solo VERSION 3.0.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.10
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(Acid303Solo
    COMPANY_NAME "Burnout Theory"
    BUNDLE_ID "info.acid303.solo"
    PLUGIN_MANUFACTURER_CODE Ac03
    PLUGIN_CODE A303
    FORMATS VST3
    PRODUCT_NAME "Acid303 Solo"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_CATEGORIES Instrument Synth
)
juce_generate_juce_header(Acid303Solo)
target_sources(Acid303Solo PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
)
target_compile_definitions(Acid303Solo PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)
target_link_libraries(Acid303Solo PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
    juce::juce_gui_extra
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_warning_flags
)

# Personal presets are stored in the OS-resolved Documents/Acid303/presets
# directory, not bundled inside the VST3 under protected Program Files.

# Preserve accented French/English UI text when compiling with MSVC.
if(MSVC)
    target_compile_options(Acid303Solo PRIVATE /utf-8)
endif()

# V3 official UPGRADE intentionally reuses the v2.0.6 VST3 identity for FL Studio project recall.
juce_add_binary_data(Acid303UpgradeKnobs SOURCES Assets/TB303_Knob.png)
target_link_libraries(Acid303Solo PRIVATE Acid303UpgradeKnobs)
