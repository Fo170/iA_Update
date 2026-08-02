#ifndef APPCONFIG_HPP
#define APPCONFIG_HPP

#include <QString>

// ── Nom de l'application ─────────────────────────────────────────────────
#define APP_NAME "iA_Update"

// ── Version de l'application ─────────────────────────────────────────────
// Modifier ici à chaque release (format X.Y.Z)
#ifndef APP_VERSION
#define APP_VERSION "1.0.0"
#endif

// ── URLs ─────────────────────────────────────────────────────────────────
// URL du fichier version.json pour la vérification de mise à jour
// Doit pointer vers un fichier JSON avec les champs : version, url, notes
#define UPDATE_CHECK_URL "https://raw.githubusercontent.com/Fo170/iA_Update/main/version.json"

// URL du dépôt GitHub (page d'accueil, releases, etc.)
#define APP_HOMEPAGE_URL "https://github.com/Fo170/iA_Update"

// URL de base pour les fichiers de langue (hébergés sur GitHub raw)
// L'application peut télécharger un fichier .txt manquant via cette URL
#define LANG_BASE_URL "https://raw.githubusercontent.com/Fo170/iA_Update/main/lang/"

// URL de base du manifeste des applications (apps.json)
#define APPS_MANIFEST_URL "https://raw.githubusercontent.com/Fo170/iA_Update/main/apps.json"

// ── Icônes ───────────────────────────────────────────────────────────────
// Toutes les icônes sont dans le dossier "ico/" à la racine du projet.
// Formats supportés :
//   - PNG (32, 64, 128, 256 px) → Qt resources (barre de titre, tâche, Alt+Tab)
//   - ICO (32, 64 px)           → Ressource Windows PE (Explorateur de fichiers)
//   - SVG (optionnel)           → Linux .desktop, thèmes d'icônes
//
// Le dossier "ico/" contient :
//   app-32.png     Icône Qt 32px
//   app-64.png     Icône Qt 64px
//   app-128.png    Icône Qt 128px
//   app-256.png    Icône Qt 256px
//   app.ico        Icône Windows (32+64px)
//   app.svg        Icône vectorielle (optionnel, Linux)
//
// Préfixe Qt resource : ":/ico/"

// ── Langues ───────────────────────────────────────────────────────────────
// Les fichiers de langue sont dans le dossier "lang/" à côté de l'exécutable.
//   lang/francais.txt     Français (inclut les émoji de l'interface)
//   lang/anglais.txt      English
//
// Format : une entrée par ligne, encodage UTF-8
//   # commentaire
//   cle=valeur
//
// L'utilisateur peut ajouter une nouvelle langue en créant un fichier .txt
// dans le dossier lang/. Le nom du fichier (sans extension) est le code
// langue. Exemple : "espanol.txt" → "Espagnol" disponible dans le menu.
//
// Auto-téléchargement : si un fichier .txt est manquant, l'application le
// télécharge depuis LANG_BASE_URL (voir ci-dessus). Aucune recompilation
// nécessaire si le fichier est déjà présent sur le dépôt distant.
//
// Les réglages sont sauvegardés dans "application.ini" (fichier INI éditable)
// à côté de l'exécutable :
//   [General]
//   langue=francais
//   geometry=...
// Le fichier .ini est recréé automatiquement s'il est supprimé.

#endif
