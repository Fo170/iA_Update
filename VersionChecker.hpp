#ifndef VERSIONCHECKER_HPP
#define VERSIONCHECKER_HPP

#include <QObject>
#include <QList>
#include <QHash>
#include <QDateTime>
#include "AppItem.hpp"

class QNetworkAccessManager;
class QNetworkReply;

// Vérifie les versions en ligne de toutes les applications surveillées,
// en parallèle et de manière asynchrone. Ne bloque jamais le thread UI.
// Utilise un cache local (cache_online.json) pour éviter d'épuiser les
// quotas d'API (GitHub : 60 requêtes/heure non authentifiées).
class VersionChecker : public QObject {
    Q_OBJECT
public:
    explicit VersionChecker(QObject* parent = nullptr);

    // Lance la vérification de toutes les applications dont online.type != none.
    // Les apps sans source en ligne sont marquées "non vérifiée".
    // Les apps en cache (moins de cacheAgeH heures) ne sont pas re-requêtées.
    void checkAll(QList<AppItem>& items, int cacheAgeH = 24);

    bool isChecking() const;

    // Force la ré-requête même si le cache est frais
    void forceRefresh(QList<AppItem>& items);
    // Retourne l'URL de fallback RSS/atom pour les sources GitHub
    // (ne consomme pas le quota API). Vide sinon.
    static QString githubAtomUrl(const AppItem& item);

signals:
    void appChecked(const QString& appId);          // une app vient d'être vérifiée
    void allChecked();                              // toutes les apps sont vérifiées
    void checkError(const QString& appId, const QString& errorMessage);

private slots:
    void onReplyFinished(QNetworkReply* reply);

private:
    QUrl buildUrl(const AppItem& item) const;
    QString parseVersion(const AppItem& item, const QByteArray& data) const;
    void loadCache();
    void saveCache() const;

    QNetworkAccessManager* manager_;
    QList<AppItem*> pending_;
    QHash<QString, QString> cacheVersion_;
    QHash<QString, qint64> cacheTime_;
    bool checking_ = false;
    bool force_ = false;
    int running_ = 0;
};

#endif
