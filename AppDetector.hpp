#ifndef APPDETECTOR_HPP
#define APPDETECTOR_HPP

#include "AppItem.hpp"

// Détecte la présence locale des applications (binaire, chemin, PATH)
// et lit leur version locale via les commandes configurées, le registre
// Windows ou le chemin d'installation.
class AppDetector {
public:
    AppDetector();

    // Analyse une application et met à jour les champs résultats.
    void detect(AppItem& item) const;

    static bool isWindows();

private:
    // Exécute un programme + arguments et capture sa sortie (sans résolution)
    QString runProgram(const QString& program, const QStringList& args,
                       const QString& fallbackLine = QString()) const;
    // Cherche le binaire dans le PATH (where/Get-Command Win, which Linux)
    QString findInPath(const QString& cmd) const;
    // Cherche dans les chemins d'installation configurés
    QString findInPaths(const QStringList& paths) const;
    // Développe le premier segment wildcard d'un chemin
    QString expandFirstWildcard(const QString& path) const;
    // Exécute une commande et capture sa sortie (stdout + stderr)
    // Gère les .cmd/.bat sous Windows via cmd.exe
    QString runCommand(const QStringList& args) const;
    // Extrait la version locale via la regex configurée
    QString extractVersion(const QString& output, const QString& regex) const;
    // Lit la version depuis le registre Windows (DisplayVersion)
    QString registryVersion(const QString& displayNamePattern) const;
    // Détermine la portée (globale/utilisateur) selon le chemin
    AppScope inferScope(const AppItem& item) const;
};

#endif
