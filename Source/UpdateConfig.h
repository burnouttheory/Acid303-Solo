#pragma once
// ACID303 SOLO - A CONFIGURER UNE FOIS AVANT LA PREMIERE DISTRIBUTION
// Il doit s'agir de l'URL HTTPS DIRECTE d'un document JSON public.
// Exemple : https://votre-domaine.example/acid303/latest.json
// Une page HTML de partage OneDrive n'est PAS une URL JSON directe.
namespace acid303update {
inline const char* manifestUrl() {
    return "https://VOTRE-ADRESSE-HTTPS/latest.json";
}
}
