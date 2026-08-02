# AGENTS.md

## What this repo is
- French documentation workspace for the `#REGLAGES_iA` Qt6 C++ GUI convention set (Windows/Linux). Not a code project: it contains only `README_INIT.md` and `00_INITIALISATION.md` — exact copies of the same files in the canonical source below.
- Canonical source: `C:\Users\Admin\SynologyDrive\[WorkSpace]\#iA\#REGLAGES_iA`. Everything the README references (`GUIDES/`, `REFERENCE/`, `templates/`) plus the reference app `ApplicationVide/` (a git repo with its own `AGENTS.md`) lives there, not here.
- No git repo, build system, tests, lint, or CI here. Verify doc work by reading the `.md` files.

## Environment gotchas (Windows, PowerShell 5.1)
- **The shell's default CWD is `C:\WINDOWS\System32\WindowsPowerShell\v1.0`, not the repo.** The `workdir` parameter is ignored. Run `Set-Location -LiteralPath "C:\Users\Admin\SynologyDrive\[WorkSpace]\#iA\iA_Update"` at the start of every shell command; otherwise relative file operations hit Windows system files (you will see `powershell.exe`, `*.ps1xml`, `Modules/`, etc.).
- **Always use `-LiteralPath`** for file commands. The repo path contains `[ ]` (wildcard chars), `#`, and spaces; `-Path`/unquoted paths silently misparse.
- Glob/Read/Write tools work fine with the absolute repo path; this trap affects only shell (`bash`) commands.

## Working here
- Real project files: `README_INIT.md`, `00_INITIALISATION.md`. Docs are written in French.
- Strict reading order 00 → 13 defined in `00_INITIALISATION.md` §5; keep `README_INIT.md`'s table of guides in sync with it.
- If changes must reach the real docs, mirror them into `#REGLAGES_iA` (currently identical).

## Conventions if you touch C++/CMake (from the docs)
- C++17, Qt6 Widgets + Network, CMake ≥ 3.16, `CMAKE_AUTOMOC`/`CMAKE_AUTORCC`; PascalCase classes, snake_case files.
- Central config lives in `AppConfig.hpp`; `APP_VERSION` is defined only there.
- Full build, deploy, and API conventions: read `#REGLAGES_iA\ApplicationVide\AGENTS.md`.
