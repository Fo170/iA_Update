#ifndef INSTALLER_HPP
#define INSTALLER_HPP

#include <QObject>
#include <QList>
#include "AppItem.hpp"

class QProcess;

// Lance l'installation automatique des applications sélectionnées.
// Windows : demande d'élévation admin via runas. Linux : requiert sudo.
// Les commandes proviennent du champ updateCommand du manifeste apps.json.
class Installer : public QObject {
    Q_OBJECT
public:
    explicit Installer(QObject* parent = nullptr);

    void install(const QList<AppItem*>& items);
    // Réparation : exécute la repairCommand pour les apps sélectionnées
    // (réinstallation, suppression de vestiges, remise en place, etc.)
    void repair(const QList<AppItem*>& items);

signals:
    void installStarted(const QString& appId);
    void installFinished(const QString& appId, bool success, const QString& output);
    void allInstalled();

private slots:
    void onProcessFinished();

private:
    QList<AppItem*> queue_;
    QProcess* process_ = nullptr;
    QString currentAppId_;
    QString currentOutput_;
    int remaining_ = 0;
    bool repairMode_ = false;
    void startNext();
};

#endif
