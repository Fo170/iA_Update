# AGENTS.md

## What this repo is
- **`iA_Update`** : application Qt6 C++17 GUI (Windows/Linux) qui détecte l'installation et les mises à jour de **52 applications** IA/outils de dev, avec actions en phase 2 (télécharger / créer script de commande / installer / réparer).
- Also contains doc copies `README_INIT.md` + `00_INITIALISATION.md` (French, from the canonical `#REGLAGES_iA` reference set). Keep these in sync with `C:\Users\Admin\SynologyDrive\[WorkSpace]\#iA\#REGLAGES_iA` if edited.
- Git repo, remote `origin` = `https://github.com/Fo170/iA_Update` (branch `main`). GPL-3.0 license.
- No tests, lint, or CI. Verify by building and launching the app (see sanity check below).

## Build (Windows, MinGW) — verified on this machine
Qt install: `C:/Qt/6.11.0/mingw_64`, MinGW: `C:/Qt/Tools/mingw1310_64`, CMake: `C:/Qt/Tools/CMake_64/bin/cmake.exe`.
```powershell
& "C:/Qt/Tools/CMake_64/bin/cmake.exe" -S . -B windows -G "MinGW Makefiles" `
  -DCMAKE_PREFIX_PATH=C:/Qt/6.11.0/mingw_64 `
  -DCMAKE_CXX_COMPILER=C:/Qt/Tools/mingw1310_64/bin/g++.exe `
  -DCMAKE_MAKE_PROGRAM=C:/Qt/Tools/mingw1310_64/bin/mingw32-make.exe
& "C:/Qt/Tools/CMake_64/bin/cmake.exe" --build windows -- -j4
```
- `cmake` is NOT on PATH. Use the full path above.
- **If `apps.json`, `lang/`, `ico/`, or `icones_app/` change, the build dir copy must be refreshed**: `cmake --build` re-runs the `file(COPY ...)` only on reconfigure. Re-copy manually, e.g. `Copy-Item -LiteralPath "<repo>\apps.json" -Destination "<repo>\windows\apps.json"`.
- Deploy: `& "C:/Qt/6.11.0/mingw_64/bin/windeployqt.exe" --no-compiler-runtime --no-translations windows/iA_Update.exe`
- **If new Qt modules are added to CMakeLists (e.g. `Concurrent`), `windeployqt` must be re-run** — otherwise the exe fails with an "entry point" error (missing DLL like `Qt6Concurrent.dll`).
- Sanity check after build: launch `windows/iA_Update.exe`; it must stay alive and create `windows/application.ini`. Kill it with `Stop-Process`.

## Environment gotchas (Windows, PowerShell 5.1)
- **The shell's default CWD is `C:\WINDOWS\System32\WindowsPowerShell\v1.0`, not the repo.** The `workdir` parameter is ignored. Use absolute paths or `Set-Location -LiteralPath "C:\Users\Admin\SynologyDrive\[WorkSpace]\#iA\iA_Update"` at the start of every shell command.
- **Always use `-LiteralPath`** for file commands. The repo path contains `[ ]` (wildcard chars), `#`, and spaces; `-Path`/unquoted paths silently misparse. `Start-Process` cannot handle this path — use `[System.Diagnostics.Process]::Start($exe)` instead.
- Glob/Read/Write tools work fine with the absolute repo path; this trap affects only shell commands.

## Architecture (key files)
| File | Role |
|---|---|
| `AppConfig.hpp` | `APP_VERSION`, URLs (`UPDATE_CHECK_URL`, `LANG_BASE_URL`, `APPS_MANIFEST_URL`). Only place APP_VERSION is defined. |
| `apps.json` | Manifeste éditable des 52 apps : `detect` (cmd/locate/paths), `versionLocal` (regex/registry/fromPath), `online` (github/npm/pypi/endoflife/none, `useTags` pour /tags), `downloadUrl`, `updateCommand` + `repairCommand` par OS, `category` (build/outils/local/assistants/code). |
| `AppItem` | Modèle d'une app + `loadManifest()`, surcharges `commandes.ini` (`applyIniOverrides`/`writeIniFile`). |
| `AppDetector` | Détection locale : PATH via `where.exe`/`which`, chemins avec wildcards, version via commande+regex, **registre Windows** (`registry`), **nom de dossier** (`fromPath`). Exécute les `.cmd` via `cmd.exe /d /c`. |
| `VersionChecker` | Vérif en ligne parallèle asynchrone. **Cache `cache_online.json`** (24 h), **fallback GitHub `releases.atom`** quand l'API est rate-limitée. |
| `Downloader` | Phase 2. Téléchargement **en 2 étapes** pour GitHub (métadonnée → asset Windows réel), nom de fichier réel, détection des liens HTML (plus de fichier corrompu). Dossier par défaut = `Downloads` de l'OS. |
| `CommandBuilder` / `Installer` | Phase 2 : script `.bat`/`.sh`, installation auto (mode `install` et `repair`). |
| `LangueManager` | i18n fr/en via `lang/*.txt` (`\n` converti en saut de ligne à la lecture). |
| `MainWindow` | Onglets par catégorie (QTabWidget + QStackedWidget), tableau triable (QSortFilterProxyModel), statuts colorés, icônes locales, onglet Réglages (commande éditable → `commandes.ini`). |

## Conventions (from the reference docs)
- C++17, Qt6 Widgets + Network + **Concurrent**, CMake ≥ 3.16, `CMAKE_AUTOMOC`/`CMAKE_AUTORCC`; PascalCase classes, snake_case files.
- `QStringLiteral(...)` for user-visible C++ strings, placeholders `%1` (never `%s`), raw pointers `= nullptr`, `reply->deleteLater()` only.
- Online sources verified: GitHub releases API (`tag_name` → strip leading `v` **only if present**), GitHub `/tags` array (first `name`) when `useTags`, npm `/<pkg>/latest` (`version`), PyPI (`info.version`), endoflife.date (**JSON array** → `[0].latest`). Don't trust endoflife for packages it lacks (anaconda 404s → use github `conda/conda`).
- **Never send the GitHub `Accept: application/vnd.github+json` header to non-GitHub endpoints** — npm registry replies 406 (this bug hit npm/n8n/node-red/claude).
- Full build/deploy/API conventions: `#REGLAGES_iA\ApplicationVide\AGENTS.md`.

## Runtime files (created next to the exe, not committed)
- `application.ini` — langue + geometry (auto-recrié).
- `cache_online.json` — versions en ligne (24 h).
- `commandes.ini` — surcharges des commandes de mise à jour/réparation, éditable manuellement ou via l'onglet Réglages.
- `download/` — dossier de téléchargement par défaut si `Downloads` OS indisponible.

## Git
- Remote set, branch `main`. Commit only when asked. Message style: French, lowercase, no accents (see `git log`).
- `.gitignore` excludes `windows/`, `linux/`, `application.ini`, `cache_online.json`, `commandes.ini`, build artifacts — do not commit the build dir or runtime files.
