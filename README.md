# iA_Update

Vérificateur d'installation et de mises à jour des applications IA et outils de développement — GUI Qt6 C++17 multiplateforme (Windows / Linux).

## 🚀 Fonctionnalités

- **Phase 1 — Diagnostic** : détecte si chaque application est installée, lit sa version locale et la compare à la dernière version en ligne.
- **Phase 2 — Actions** : pour les applications sélectionnées, choisissez une ou plusieurs actions :
  1. 📥 Télécharger la mise à jour dans un dossier choisi (défaut : `download/`)
  2. 📄 Créer un fichier de commande (`.bat`/`.sh`) avec la commande de mise à jour adaptée à l'OS
  3. ⚙️ Installer automatiquement (nécessite des droits administrateur)

## 📋 Applications surveillées

| Catégorie | Applications |
|---|---|
| Runtimes | Python, Node.js, Qt, Anaconda |
| IA locale | Ollama, LM Studio, AnythingLLM |
| IA cloud / CLI | Kimi, DeepSeek, Claude, ChatGPT (Codex), Antigravity, Perplexity, OpenCode |
| Automation | Node-RED, n8n |
| Édition / IDE | Ghostwriter, Visual Studio Code, Spyder, Obsidian |

## 🖥️ Interface

Tableau avec les colonnes : **Nom | Version locale | Version en ligne | Statut | Portée | Chemin d'installation | Référencé dans le PATH**.

Statuts colorés : 🟢 à jour · 🟠 mise à jour disponible · 🔴 non installé · ⚪ version locale inconnue · 🔵 version en ligne inconnue.

Filtres par statut, par catégorie et recherche textuelle.

## ⚙️ Configuration

La liste des applications et leurs règles de détection sont définies dans **`apps.json`** (à côté de l'exécutable), éditable sans recompiler.

```json
{
  "id": "python",
  "name": "Python",
  "category": "Runtime",
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
  }
}
```

Types de source en ligne supportés : `github` (repo), `npm` (package), `pypi` (package), `endoflife` (key), `none` (aucune source fiable → statut 🔵 avec lien de téléchargement).

## 🔨 Compilation

Prérequis : Qt 6.8+ (Widgets + Network), CMake ≥ 3.16, compilateur C++17 (MinGW / MSVC / GCC).

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

Déploiement Windows : `windeployqt windows/iA_Update.exe`

## 🌍 Langues

Français et anglais détectés automatiquement selon le système. Les fichiers de traduction sont dans `lang/` (édition possible sans recompiler).

## 📁 Structure

```
├── CMakeLists.txt          # Build CMake
├── AppConfig.hpp           # Version, URLs (config centrale)
├── MainWindow.*            # Fenêtre principale (tableau, filtres, phase 2)
├── AppItem.*               # Modèle d'une application + chargement apps.json
├── AppDetector.*           # Détection locale (PATH, chemins, version)
├── VersionChecker.*        # Vérification des versions en ligne (asynchrone)
├── Downloader.*            # Phase 2 — téléchargement
├── CommandBuilder.*        # Phase 2 — fichier de commande
├── Installer.*             # Phase 2 — installation automatique
├── LangueManager.*         # i18n fr/en
├── apps.json               # Manifeste des applications surveillées
├── lang/                   # Traductions
├── ico/                    # Icônes
└── version.json            # Manifeste de version (mises à jour de iA_Update)
```

## 📄 Licence

GPL-3.0 — voir `LICENSE`.
