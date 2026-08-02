#include "Downloader.hpp"
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include "AppConfig.hpp"

Downloader::Downloader(QObject* parent) : QObject(parent) {
    manager_ = new QNetworkAccessManager(this);
    connect(manager_, &QNetworkAccessManager::finished,
            this, &Downloader::onDownloadFinished);
}

QUrl Downloader::downloadUrlFor(const AppItem& item) const {
    // Pour les applications GitHub, on tente l'asset de la dernière release.
    // downloadUrl du manifeste est utilisé comme fallback.
    QString type = item.online.value("type").toString();
    QString repo = item.online.value("repo").toString();

    if (type == "github" && !repo.isEmpty()) {
        return QUrl(QStringLiteral("https://api.github.com/repos/%1/releases/latest").arg(repo));
    }
    return QUrl(item.downloadUrl);
}

QString Downloader::fileNameFor(const AppItem& item) const {
    return item.id + "_" + (item.online_version.isEmpty() ? QStringLiteral("latest")
                                                           : item.online_version) + ".exe";
}

void Downloader::download(const QList<AppItem*>& items, const QString& destDir) {
    destDir_ = destDir;
    QDir().mkpath(destDir_);

    for (auto* item : items) {
        if (!item->installed || item->online_version.isEmpty())
            continue;

        QUrl url = downloadUrlFor(*item);
        if (url.isEmpty())
            continue;

        QNetworkRequest request(url);
        request.setHeader(QNetworkRequest::UserAgentHeader,
            QStringLiteral(APP_NAME "/%1").arg(QStringLiteral(APP_VERSION)));
        request.setTransferTimeout(60000);

        QNetworkReply* reply = manager_->get(request);
        reply->setProperty("appId", item->id);
        reply->setProperty("destPath", destDir_ + "/" + fileNameFor(*item));
        pending_++;

        connect(reply, &QNetworkReply::downloadProgress, this,
            [this, item](qint64 received, qint64 total) {
                emit downloadProgress(item->id, received, total);
            });
    }

    if (pending_ == 0)
        emit allDownloadsFinished();
}

void Downloader::onDownloadFinished(QNetworkReply* reply) {
    reply->deleteLater();
    pending_--;

    QString appId = reply->property("appId").toString();
    QString destPath = reply->property("destPath").toString();

    bool success = false;
    if (reply->error() == QNetworkReply::NoError) {
        QFile f(destPath);
        if (f.open(QIODevice::WriteOnly)) {
            f.write(reply->readAll());
            f.close();
            success = true;
        }
    }

    emit downloadFinished(appId, success, destPath);

    if (pending_ == 0)
        emit allDownloadsFinished();
}
