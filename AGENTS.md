# AGENTS.md

## What this repo is
- **`iA_Update`** : application Qt6 C++17 GUI (Windows/Linux) qui détecte l'installation et les mises à jour de 20 applications IA/outils de dev, avec actions en phase 2 (télécharger / créer script de commande / installer auto).
- Also contains doc copies `README_INIT.md` + `00_INITIALISATION.md` (French, from the canonical `#REGLAGES_iA` reference set). Keep these in sync with `C:\Users\Admin\SynologyDrive\[WorkSpace]\#iA\#REGLAGES_iA` if edited.
- Git repo, remote `origin` = `https://github.com/Fo170/iA_Update` (branch `main`). GPL-3.0 license.
- No tests, lint, or CI. Verify by building and launching the app.

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
- **If `apps.json`, `lang/`, or `ico/` change, the build dir copy must be refreshed**: `cmake --build` re-runs the `file(COPY ...)` only on reconfigure. Re-copy manually: `Copy-Item -LiteralPath "<repo>\apps.json" -Destination "<repo>\windows\apps.json"`.
- Deploy: `& "C:/Qt/6.11.0/mingw_64/bin/windeployqt.exe" --no-compiler-runtime --no-translations windows/iA_Update.exe`
- Sanity check after build: launch `windows/iA_Update.exe`; it must stay alive and create `windows/application.ini`. Kill it with `Stop-Process`.

## Environment gotchas (Windows, PowerShell 5.1)
- **The shell's default CWD is `C:\WINDOWS\System32\WindowsPowerShell\v1.0`, not the repo.** The `workdir` parameter is ignored. Use absolute paths or `Set-Location -LiteralPath "C:\Users\Admin\SynologyDrive\[WorkSpace]\#iA\iA_Update"` at the start of every shell command.
- **Always use `-LiteralPath`** for file commands. The repo path contains `[ ]` (wildcard chars), `#`, and spaces; `-Path`/unquoted paths silently misparse. `Start-Process` cannot handle this path — use `[System.Diagnostics.Process]::Start($exe)` instead.
- Glob/Read/Write tools work fine with the absolute repo path; this trap affects only shell commands.

## Architecture (key files)
| File | Role |
|---|---|
| `AppConfig.hpp` | `APP_VERSION`, URLs (`UPDATE_CHECK_URL`, `LANG_BASE_URL`, `APPS_MANIFEST_URL`). Only place APP_VERSION is defined. |
| `apps.json` | Manifeste éditable des apps surveillées: `detect`, `versionLocal.regex`, `online` (github/npm/pypi/endoflife/none), `downloadUrl`, `updateCommand` par OS. |
| `AppItem` | Modèle d'une app + `loadManifest()`. |
| `AppDetector` | Détection locale (PATH via `where.exe`/`which`, chemins, version via commande+regex). |
| `VersionChecker` | Vérif en ligne parallèle asynchrone (QNetworkAccessManager, timeout 15 s, `deleteLater()`). |
| `Downloader` / `CommandBuilder` / `Installer` | Phase 2. |
| `LangueManager` | i18n fr/en via `lang/*.txt`. |
| `MainWindow` | Tableau, filtres, statuts colorés, phase 2. |

## Conventions (from the reference docs)
- C++17, Qt6 Widgets + Network, CMake ≥ 3.16, `CMAKE_AUTOMOC`/`CMAKE_AUTORCC`; PascalCase classes, snake_case files.
- `QStringLiteral(...)` for user-visible C++ strings, placeholders `%1` (never `%s`), raw pointers `= nullptr`, `reply->deleteLater()` only.
- Online sources verified working: GitHub releases API (`tag_name` → strip leading `v`), npm `/<pkg>/latest` (`version`), PyPI (`info.version`), endoflife.date (returns a **JSON array** — take `[0].latest`). Don't trust endoflife for packages it lacks (anaconda 404s → use github `conda/conda`).
- Full build/deploy/API conventions: `#REGLAGES_iA\ApplicationVide\AGENTS.md`.

## Git
- Remote set, branch `main`. Commit only when asked. Message style: French, lowercase, no accents (see `git log`).
- `.gitignore` excludes `windows/`, `linux/`, `application.ini`, build artifacts — do not commit the build dir.
