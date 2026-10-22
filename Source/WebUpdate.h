#pragma once
#include <JuceHeader.h>
#include "UpdateConfig.h"

// Checks ONLY a public HTTPS JSON manifest. No OneDrive account, local sync,
// file downloads, installation, admin rights or activity on the audio thread.
namespace acid303update {
inline juce::String installedVersion() { return "0.30.2"; }

inline int compareVersions(const juce::String& a, const juce::String& b) {
    const auto aa = juce::StringArray::fromTokens(a, ".", "");
    const auto bb = juce::StringArray::fromTokens(b, ".", "");
    for (int i = 0; i < 3; ++i) {
        const int x = i < aa.size() ? aa[i].getIntValue() : 0;
        const int y = i < bb.size() ? bb[i].getIntValue() : 0;
        if (x != y) return x > y ? 1 : -1;
    }
    return 0;
}

struct Offer {
    bool configured = false;
    bool valid = false;
    bool newer = false;
    juce::String version, downloadUrl, notes, error;
};

inline bool validVersion(const juce::String& str) {
    const auto bits = juce::StringArray::fromTokens(str, ".", "");
    if (bits.size() != 3) return false;
    for (const auto& b : bits)
        if (b.isEmpty() || b.length() > 5 || !b.containsOnly("0123456789")) return false;
    return true;
}

inline Offer fetchOffer() {
    Offer o;
    const juce::String address = manifestUrl();
    if (!address.startsWithIgnoreCase("https://") || address.contains("VOTRE-ADRESSE")) {
        o.error = "URL de verification non configuree.";
        return o;
    }
    o.configured = true;
    int status = 0;
    auto request = juce::URL(address);
    auto opts = juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress)
                    .withConnectionTimeoutMs(3500)
                    .withNumRedirectsToFollow(5)
                    .withStatusCode(&status);
    auto stream = request.createInputStream(opts);
    if (stream == nullptr || (status != 0 && (status < 200 || status >= 300))) {
        o.error = "Serveur de version indisponible.";
        return o;
    }
    juce::MemoryOutputStream payload;
    char buffer[2048];
    while (payload.getDataSize() <= 32768) {
        const auto count = stream->read(buffer, sizeof(buffer));
        if (count <= 0) break;
        payload.write(buffer, (size_t) count);
    }
    if (payload.getDataSize() == 0 || payload.getDataSize() > 32768) {
        o.error = "Fichier de version vide ou trop volumineux.";
        return o;
    }
    const juce::String raw = juce::String::fromUTF8(
        static_cast<const char*>(payload.getData()), (int) payload.getDataSize());
    const juce::var parsed = juce::JSON::parse(raw);
    if (!parsed.isObject()) {
        o.error = "Le lien ne retourne pas un document JSON valide.";
        return o;
    }
    o.version = parsed.getProperty("version", juce::var()).toString().trim();
    o.downloadUrl = parsed.getProperty("download_url", juce::var()).toString().trim();
    o.notes = parsed.getProperty("notes", juce::var()).toString().trim();
    if (!validVersion(o.version) || !o.downloadUrl.startsWithIgnoreCase("https://")
        || o.downloadUrl.length() > 2048) {
        o.error = "Version ou lien de telechargement invalide.";
        return o;
    }
    o.newer = compareVersions(o.version, installedVersion()) > 0;
    o.valid = true;
    return o;
}

// Owner: editor, with worker lifecycle tied to the editor lifetime.
// Results are polled by the message-thread Timer, never by the audio callback.
class CheckThread : public juce::Thread {
public:
    CheckThread() : juce::Thread("Acid303 online update check") {}
    void run() override {
        auto current = fetchOffer();
        const juce::ScopedLock lock(mutex);
        offer = current;
        ready = true;
    }
    bool takeResult(Offer& destination) {
        const juce::ScopedLock lock(mutex);
        if (!ready) return false;
        destination = offer;
        ready = false;
        return true;
    }
private:
    juce::CriticalSection mutex;
    Offer offer;
    bool ready = false;
};
} // namespace acid303update
