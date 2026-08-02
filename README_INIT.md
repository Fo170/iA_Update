# Documentation Qt6 GUI — Dossier de référence REGLAGES_iA

> **Fiche d'identité**
> - **Version** : 1.0
> - **Dernière modification** : 2026-07-23
> - **Compatibilité** : Qt 6.8 à Qt 6.11+

Bienvenue dans le manuel de développement pour applications **Qt6 C++ Multiplateformes (Windows / Linux)**.

---

## 🚀 Par quoi commencer ?

- **Créer une nouvelle application** : Consulter [00_INITIALISATION.md](00_INITIALISATION.md).
- **Comprendre la structure globale** : Consulter [01_ARCHITECTURE.md](GUIDES/01_ARCHITECTURE.md).

---

## 📚 Guides d'implantation (GUIDES/)

| Fichier | Sujet |
|---|---|
| [01_ARCHITECTURE.md](GUIDES/01_ARCHITECTURE.md) | Structure, CMake, Build Linux/Windows |
| [02_QT_ENVIRONNEMENT.md](GUIDES/02_QT_ENVIRONNEMENT.md) | Modules CMake, classes Qt utilisées |
| [03_ICONES.md](GUIDES/03_ICONES.md) | Gestion des icônes multi-tailles et Windows PE |
| [04_LANGUES.md](GUIDES/04_LANGUES.md) | Système i18n et langues dynamiques |
| [05_EMOJIS_ET_TEXTE.md](GUIDES/05_EMOJIS_ET_TEXTE.md) | Encodage UTF-8, `QStringLiteral` et émojis couleur |
| [06_MISES_A_JOUR.md](GUIDES/06_MISES_A_JOUR.md) | Système de mise à jour via GitHub |
| [07_FICHIERS_INI.md](GUIDES/07_FICHIERS_INI.md) | Sauvegarde `QSettings` (`application.ini`) |
| [08_PROJET_JSON.md](GUIDES/08_PROJET_JSON.md) | Chargement et sauvegarde de projets JSON |
| [09_BOITE_A_PROPOS.md](GUIDES/09_BOITE_A_PROPOS.md) | Fenêtre "À propos" standardisée |

---

## 🔍 Référence & Débogage (REFERENCE/)

| Fichier | Sujet |
|---|---|
| [10_PIEGES_ET_DEBUG.md](REFERENCE/10_PIEGES_ET_DEBUG.md) | Résolution de bugs, OpenSSL, DLL, crashs |
| [11_DEPLOIEMENT.md](REFERENCE/11_DEPLOIEMENT.md) | Deploy Windows (`windeployqt`), NSIS, Linux |
| [12_CHECKLIST_FINALE.md](REFERENCE/12_CHECKLIST_FINALE.md) | Validation finale avant release |
| [13_GUIDE_INSTALLATION_QT6.md](REFERENCE/13_GUIDE_INSTALLATION_QT6.md) | Guide d'installation — Stack Qt6 C++17 (Widgets + Network) |

---

## 📋 Modèles de suivi (templates/)

- `templates/info_projet.md`
- `templates/plan_et_etapes.md`
- `templates/memorie.md`