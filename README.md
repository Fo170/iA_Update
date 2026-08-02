# iA_Update

Vérificateur d'installation et de mises à jour des applications IA et outils de développement — GUI Qt6 C++17 multiplateforme (Windows / Linux).

## 🚀 Fonctionnalités

- **Phase 1 — Diagnostic** : détecte si chaque application est installée, lit sa version locale (commande, registre Windows ou nom de dossier) et la compare à la dernière version en ligne.
- **Phase 2 — Actions** : pour les applications sélectionnées (cases à cocher), choisissez une ou plusieurs actions :
  1. 📥 Télécharger la mise à jour (dossier `Téléchargements` de l'OS par défaut, téléchargement de l'installateur réel)
  2. 📄 Créer un fichier de commande (`.bat`/`.sh`) avec la commande adaptée à l'OS
  3. ⚙️ Installer automatiquement (nécessite des droits administrateur)
  4. 🔧 Réparer une installation incorrecte (réinstallation, remise en place, suppression des vestiges)

## 📋 Applications surveillées (52)

| Onglet | Applications |
|---|---|
| 🛠️ Outils de build | CMake, MinGW (GCC), Git, Ninja |
| 🔧 Outils & Runtimes | Python, Node.js, npm, Qt, Anaconda, Ghostwriter, Obsidian, Node-RED, n8n, Winget, pip, Chocolatey, Scoop, .NET SDK, Rust/Cargo |
| 🧠 Local — modèles | Ollama, LM Studio, AnythingLLM, Jan, GPT4All, Chatbox, Cherry Studio, Msty, ComfyUI |
| 🤖 Assistants chat | Kimi, Perplexity, ChatGPT (Desktop), Claude (Desktop), Le Chat, Grok, Gemini, Pi, Qwen, Antigravity |
| 💻 IA de code | VS Code, Spyder, Zed, Cursor, Windsurf, Trae, OpenCode, OpenFox, Gemini CLI, Copilot CLI, aider, Continue, Claude (CLI), Codex |

## 🖥️ Interface

- **Onglets par catégorie** (Tous / Build / Outils / Local / Assistants / Code / Réglages).
- Tableau avec les colonnes : **Icône | Nom | Version locale | Version en ligne | Statut | Portée | Chemin d'installation | Dans PATH**.
- Statuts colorés : 🟢 à jour · 🟠 mise à jour disponible · 🔴 non installé · ⚪ version locale inconnue · 🔵 version en ligne inconnue.
- Icônes des applications affichées (dossier `icones_app/`).
- Tri des colonnes (numérique pour les versions), filtres par statut et recherche textuelle.
- **Onglet ⚙️ Réglages** : liste des applications avec leurs commandes de mise à jour/réparation modifiables (Windows/Linux), sauvegardées dans `commandes.ini`.

## ⚙️ Configuration

La liste des applications et leurs règles de détection sont définies dans **`apps.json`** (à côté de l'exécutable), éditable sans recompiler.

```json
{
  "id": "python",
  "name": "Python",
  "category": "outils",
  "detect": {
    "windows": { "cmd": ["py", "--version"], "paths": ["C:/Python*"] },
    "linux": { "cmd": ["python3", "--version"] }
  },
  "versionLocal": { "regex": "(\\d+\\.\\d+\\.\\d+)" },
  "online": { "type": "endoflife", "key": "python" },
  "downloadUrl": "https://www.python.org/downloads/",
  "updateCommand": {
    "windows": "winget upgrade --id Python.Python.3.12",
    "linux": "sudo apt update && sudo apt upgrade python3"
  },
  "repairCommand": {
    "windows": "winget install --id Python.Python.3.12 --force",
    "linux": "sudo apt update && sudo apt install --reinstall python3"
  }
}
```

- **`versionLocal`** : `regex` (sortie de commande), `registry` (registre Windows), `fromPath` (nom de dossier).
- **`online`** : `github` (repo, option `useTags` pour l'endpoint `/tags`), `npm` (package), `pypi` (package), `endoflife` (key), `none` (aucune source fiable → statut 🔵 avec lien).
- **`commandes.ini`** : surcharges utilisateur des commandes (généré automatiquement au 1er lancement, modifiable dans l'onglet Réglages ou manuellement).

## 🔨 Compilation

Prérequis : Qt 6.8+ (Widgets + Network + Concurrent), CMake ≥ 3.16, compilateur C++17 (MinGW / MSVC / GCC).

```bash
# Windows (MinGW)
cmake -S . -B windows -G "MinGW Makefiles" \
  -DCMAKE_PREFIX_PATH=C:/Qt/6.x.x/mingw_64 \
  -DCMAKE_CXX_COMPILER=C:/mingw64/bin/g++.exe \
  -DCMAKE_MAKE_PROGRAM=C:/mingw64/bin/mingw32-make.exe
cmake --build windows

# Linux
cmake -S . -B linux -DCMAKE_PREFIX_PATH=/opt/Qt/6.x.x/gcc_64
cmake --build linux
```

Déploiement Windows : `windeployqt windows/iA_Update.exe` (à relancer si un module Qt est ajouté au CMake, ex. `Concurrent`).

## 🌍 Langues

Français et anglais détectés automatiquement selon le système. Les fichiers de traduction sont dans `lang/` (édition possible sans recompiler).

## 📁 Structure

```
├── CMakeLists.txt          # Build CMake
├── AppConfig.hpp           # Version, URLs (config centrale)
├── MainWindow.*            # Fenêtre principale (onglets, tableau, filtres, phase 2, réglages)
├── AppItem.*               # Modèle d'une application + chargement apps.json / commandes.ini
├── AppDetector.*           # Détection locale (PATH, chemins, registre, version)
├── VersionChecker.*        # Vérification des versions en ligne (asynchrone + cache)
├── Downloader.*            # Phase 2 — téléchargement (2 étapes GitHub)
├── CommandBuilder.*        # Phase 2 — fichier de commande
├── Installer.*             # Phase 2 — installation / réparation automatique
├── LangueManager.*         # i18n fr/en
├── apps.json               # Manifeste des applications surveillées
├── lang/                   # Traductions
├── ico/                    # Icône de l'application iA_Update
├── icones_app/             # Icônes des applications surveillées
└── version.json            # Manifeste de version (mises à jour de iA_Update)
```

## 📄 Licence

GPL-3.0 — voir `LICENSE`.
