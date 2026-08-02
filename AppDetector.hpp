#ifndef APPDETECTOR_HPP
#define APPDETECTOR_HPP

#include "AppItem.hpp"

// Détecte la présence locale des applications (binaire, chemin, PATH)
// et lit leur version locale via les commandes configurées.
class AppDetector {
public:
    AppDetector();

    // Analyse une application et met à jour les champs résultats.
    // Utilise la commande de détection si configurée, sinon les chemins.
    void detect(AppItem& item) const;

    static bool isWindows();

private:
    // Cherche le binaire dans le PATH (where/Get-Command Win, which Linux)
    QString findInPath(const QString& cmd) const;
    // Cherche dans les chemins d'installation configurés
    QString findInPaths(const QStringList& paths) const;
    // Exécute une commande et capture sa sortie (stdout + stderr)
    QString runCommand(const QStringList& args) const;
    // Extrait la version locale via la regex configurée
    QString extractVersion(const QString& output, const QString& regex) const;
    // Détermine la portée (globale/utilisateur) selon le chemin
    AppScope inferScope(const AppItem& item) const;
};

#endif
