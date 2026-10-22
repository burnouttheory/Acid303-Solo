#pragma once
// ACID303 SOLO - Source publique des annonces de versions.
// Fichier brut JSON dans le depot GitHub officiel.
// Pas de cle API, de compte GitHub ni de dossier OneDrive necessaire.
// ATTENTION : ce depot doit rester public pour que la verification fonctionne.
namespace acid303update {
inline const char* manifestUrl() {
    return "https://raw.githubusercontent.com/burnouttheory/Acid303-Solo/main/Publication/latest.json";
}
}
