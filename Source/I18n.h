#pragma once
#include <JuceHeader.h>
#include <atomic>
#include <mutex>

// UI language is a global user preference, not a synth parameter or a preset.
// Keeping it outside the APVTS preserves compatibility with existing FL projects.
namespace acid303i18n {
enum class Language : int { French = 1, English = 2 };
inline juce::File preferenceFile() {
    return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
        .getChildFile("Acid303").getChildFile("language.txt");
}
inline std::atomic<int>& languageStorage() {
    static std::atomic<int> language{static_cast<int>(Language::French)};
    return language;
}
inline void ensureLoaded() {
    static std::once_flag once;
    std::call_once(once, [] {
        auto lang = juce::SystemStats::getUserLanguage().startsWithIgnoreCase("fr")
            ? Language::French : Language::English;
        const auto file = preferenceFile();
        if (file.existsAsFile()) {
            const auto stored = file.loadFileAsString().trim().toLowerCase();
            if (stored == "fr") lang = Language::French;
            else if (stored == "en") lang = Language::English;
        }
        languageStorage().store(static_cast<int>(lang), std::memory_order_release);
    });
}
inline Language current() {
    ensureLoaded();
    return static_cast<Language>(languageStorage().load(std::memory_order_acquire));
}
inline void select(Language language) {
    ensureLoaded();
    languageStorage().store(static_cast<int>(language), std::memory_order_release);
    const auto file = preferenceFile();
    if (file.getParentDirectory().createDirectory().wasOk())
        file.replaceWithText(language == Language::French ? "fr\n" : "en\n");
}
inline juce::String tr(const char* french, const char* english) {
    return current() == Language::French ? juce::String::fromUTF8(french)
                                         : juce::String::fromUTF8(english);
}
}
