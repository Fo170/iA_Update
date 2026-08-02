# Initialisation Projet Qt GUI

> **Fiche d'identité**
> - **Version** : 1.0
> - **Dernière modification** : 2026-07-23
> - **Compatibilité** : Qt 6.8+, Qt 6.9+, Qt 6.10+, Qt 6.11+

## 1. Objectif
Ce document est le point d'entrée unique pour initialiser et configurer tout nouveau projet d'application Qt6 GUI C++ multiplateforme (Windows / Linux).

## 2. Quand utiliser ce document ?
À utiliser impérativement **au tout début** de la création d'un projet avant d'écrire du code ou de configurer les scripts de build.

## 3. Prérequis
- Dossier de référence `REGLAGES_iA` accessible sur votre machine.
  - **Linux** : `/home/administrateur/SynologyDrive/[WorkSpace]/#iA/#REGLAGES_iA`
  - **Windows** : `C:\Users\Admin\SynologyDrive\[WorkSpace]\#iA\#REGLAGES_iA`
- Qt 6.8+ et CMake 3.16+ installés.

---

## Voir aussi
[README_INIT.md](README_INIT.md) | [01_ARCHITECTURE.md](GUIDES/01_ARCHITECTURE.md) →

---

## 4. Mise en œuvre
Pour créer un nouveau projet :
1. Copier ce fichier `00_INITIALISATION.md` à la racine de votre nouveau projet.
2. Créer l'arborescence recommandée (voir [01_ARCHITECTURE.md](GUIDES/01_ARCHITECTURE.md)).
3. Suivre l'ordre de lecture de la documentation répertoriée ci-dessous.

## 5. Ordre de lecture de la documentation

| # | Fichier | Objectif | Recompilation si modifié ? |
|---|---------|----------|:--------------------------:|
| 00 | `00_INITIALISATION.md` | Point d'entrée unique | — |
| 01 | `GUIDES/01_ARCHITECTURE.md` | Structure projet, CMake, compilation multi-OS | Oui (CMake) |
| 02 | `GUIDES/02_QT_ENVIRONNEMENT.md` | Classes Qt utilisées, modules CMake | Oui (CMake) |
| 03 | `GUIDES/03_ICONES.md` | Création et intégration des icônes | Oui (ressources) |
| 04 | `GUIDES/04_LANGUES.md` | Système de traduction i18n | non |
| 05 | `GUIDES/05_EMOJIS_ET_TEXTE.md` | Encodage UTF-8, QStringLiteral, émojis | Oui (C++) |
| 06 | `GUIDES/06_MISES_A_JOUR.md` | Système de mise à jour GitHub | Oui (C++) |
| 07 | `GUIDES/07_FICHIERS_INI.md` | Gestion de la configuration QSettings | non |
| 08 | `GUIDES/08_PROJET_JSON.md` | Chargement / Sauvegarde JSON | Oui (C++) |
| 09 | `GUIDES/09_BOITE_A_PROPOS.md` | Fenêtre À propos standardisée | Oui (C++) |
| 10 | `REFERENCE/10_PIEGES_ET_DEBUG.md` | Résolution de pannes & erreurs fréquentes | — |
| 11 | `REFERENCE/11_DEPLOIEMENT.md` | Procédure de déploiement (windeployqt) | non |
| 12 | `REFERENCE/12_CHECKLIST_FINALE.md` | Validation globale avant release | — |
| 13 | `REFERENCE/13_GUIDE_INSTALLATION_QT6.md` | Guide d'installation — Stack Qt6 C++17 (Widgets + Network) | — |

## 6. Bonnes pratiques
- Toujours vérifier l'accès au dossier de référence `REGLAGES_iA`.
- Définir `APP_VERSION` uniquement dans `AppConfig.hpp`.

## 7. Pièges fréquents
> Pour une liste exhaustive des erreurs, voir [10_PIEGES_ET_DEBUG.md](REFERENCE/10_PIEGES_ET_DEBUG.md).

---

## À retenir
✓ Conserver une copie de `00_INITIALISATION.md` à la racine du projet.
✓ Utiliser `AppConfig.hpp` pour toute constante globale.
✓ Se référer aux guides dans l'ordre indiqué.
