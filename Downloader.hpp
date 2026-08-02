#ifndef DOWNLOADER_HPP
#define DOWNLOADER_HPP

#include <QObject>
#include <QList>
#include <QString>
#include "AppItem.hpp"

class QNetworkAccessManager;
class QNetworkReply;

// Télécharge les installateurs/mises à jour sélectionnés dans un dossier
// choisi par l'utilisateur (défaut : ./download). Asynchrone et parallèle.
class Downloader : public QObject {
    Q_OBJECT
public:
    explicit Downloader(QObject* parent = nullptr);

    void download(const QList<AppItem*>& items, const QString& destDir);

signals:
    void downloadProgress(const QString& appId, qint64 received, qint64 total);
    void downloadFinished(const QString& appId, bool success, const QString& filePath);
    void allDownloadsFinished();

private slots:
    void onDownloadFinished(QNetworkReply* reply);

private:
    QUrl downloadUrlFor(const AppItem& item) const;
    QString fileNameFor(const AppItem& item) const;

    QNetworkAccessManager* manager_;
    QString destDir_;
    int pending_ = 0;
};

#endif
