#ifndef COMMANDBUILDER_HPP
#define COMMANDBUILDER_HPP

#include <QString>
#include <QList>
#include "AppItem.hpp"

// Génère un fichier de script (Windows .bat / Linux .sh) contenant les
// commandes de mise à jour des applications sélectionnées. L'utilisateur
// l'exécute ensuite lui-même avec les droits requis.
class CommandBuilder {
public:
    CommandBuilder();

    // Retourne le contenu du script pour les apps sélectionnées.
    QString buildScript(const QList<AppItem*>& items) const;

    // Extensions de fichier : .bat (Windows), .sh (Linux)
    static QString scriptExtension();
    static bool isWindows();
};

#endif
