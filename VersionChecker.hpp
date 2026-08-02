#ifndef VERSIONCHECKER_HPP
#define VERSIONCHECKER_HPP

#include <QObject>
#include <QList>
#include "AppItem.hpp"

class QNetworkAccessManager;
class QNetworkReply;

// Vérifie les versions en ligne de toutes les applications surveillées,
// en parallèle et de manière asynchrone. Ne bloque jamais le thread UI.
class VersionChecker : public QObject {
    Q_OBJECT
public:
    explicit VersionChecker(QObject* parent = nullptr);

    // Lance la vérification de toutes les applications dont online.type != none.
    // Les apps sans source en ligne sont marquées "non vérifiée".
    void checkAll(QList<AppItem>& items);

    bool isChecking() const;

signals:
    void appChecked(const QString& appId);          // une app vient d'être vérifiée
    void allChecked();                              // toutes les apps sont vérifiées
    void checkError(const QString& appId, const QString& errorMessage);

private slots:
    void onReplyFinished(QNetworkReply* reply);

private:
    QUrl buildUrl(const AppItem& item) const;
    QString parseVersion(const AppItem& item, const QByteArray& data) const;

    QNetworkAccessManager* manager_;
    QList<AppItem*> pending_;
    bool checking_ = false;
    int running_ = 0;
};

#endif
