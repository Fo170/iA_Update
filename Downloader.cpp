#include "Downloader.hpp"
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
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

// URL de la métadonnée de la dernière release GitHub (JSON avec les assets)
QUrl Downloader::downloadUrlFor(const AppItem& item) const {
    QString type = item.online.value("type").toString();
    QString repo = item.online.value("repo").toString();

    if (type == "github" && !repo.isEmpty()) {
        return QUrl(QStringLiteral("https://api.github.com/repos/%1/releases/latest").arg(repo));
    }
    return QUrl(item.downloadUrl);
}

// Nom du fichier de destination
QString Downloader::fileNameFor(const AppItem& item) const {
    return item.id + "_" + (item.online_version.isEmpty() ? QStringLiteral("latest")
                                                           : item.online_version) + ".exe";
}

void Downloader::download(const QList<AppItem*>& items, const QString& destDir) {
    destDir_ = destDir;
    QDir().mkpath(destDir_);

    for (auto* item : items) {
        if (item->online_version.isEmpty() && item->downloadUrl.isEmpty())
            continue;

        QUrl url = downloadUrlFor(*item);
        if (url.isEmpty())
            continue;

        QString destPath = destDir_ + "/" + fileNameFor(*item);

        if (item->online.value("type").toString() == "github") {
            // Étape 1 : récupère la métadonnée de la release (JSON)
            QNetworkRequest request(url);
            request.setHeader(QNetworkRequest::UserAgentHeader,
                QStringLiteral(APP_NAME "/%1").arg(QStringLiteral(APP_VERSION)));
            request.setTransferTimeout(30000);
            request.setRawHeader("Accept", "application/vnd.github+json");
            QNetworkReply* reply = manager_->get(request);
            reply->setProperty("appId", item->id);
            reply->setProperty("destPath", destPath);
            reply->setProperty("step", QStringLiteral("metadata"));
            pending_++;
        } else {
            // Autres sources : téléchargement direct
            QNetworkRequest request(url);
            request.setHeader(QNetworkRequest::UserAgentHeader,
                QStringLiteral(APP_NAME "/%1").arg(QStringLiteral(APP_VERSION)));
            request.setTransferTimeout(60000);
            QNetworkReply* reply = manager_->get(request);
            reply->setProperty("appId", item->id);
            reply->setProperty("destPath", destPath);
            reply->setProperty("step", QStringLiteral("direct"));
            pending_++;
        }
    }

    if (pending_ == 0)
        emit allDownloadsFinished();
}

void Downloader::onDownloadFinished(QNetworkReply* reply) {
    reply->deleteLater();
    pending_--;

    QString appId = reply->property("appId").toString();
    QString destPath = reply->property("destPath").toString();
    QString step = reply->property("step").toString();

    if (reply->error() != QNetworkReply::NoError) {
        emit downloadFinished(appId, false, destPath);
        if (pending_ == 0)
            emit allDownloadsFinished();
        return;
    }

    QByteArray data = reply->readAll();

    // Étape métadonnée GitHub : choisir l'asset Windows et télécharger
    if (step == QStringLiteral("metadata")) {
        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(data, &err);
        if (err.error == QJsonParseError::NoError && doc.isObject()) {
            QJsonArray assets = doc.object().value("assets").toArray();
            QString assetUrl;
            // Préférence d'extension pour Windows
            for (const QString& ext : {QStringLiteral(".exe"), QStringLiteral(".msi"),
                                       QStringLiteral(".zip"), QStringLiteral(".7z")}) {
                for (const auto& a : assets) {
                    QString name = a.toObject().value("name").toString();
                    if (name.endsWith(ext, Qt::CaseInsensitive) &&
                        !name.contains("debug", Qt::CaseInsensitive)) {
                        assetUrl = a.toObject().value("browser_download_url").toString();
                        break;
                    }
                }
                if (!assetUrl.isEmpty())
                    break;
            }
            // Si aucun asset trouvé, on garde la page de téléchargement du manifeste
            if (assetUrl.isEmpty()) {
                assetUrl = QString();
            }
            if (!assetUrl.isEmpty()) {
                QNetworkRequest request{QUrl(assetUrl)};
                request.setHeader(QNetworkRequest::UserAgentHeader,
                    QStringLiteral(APP_NAME "/%1").arg(QStringLiteral(APP_VERSION)));
                request.setTransferTimeout(120000);
                QNetworkReply* r2 = manager_->get(request);
                r2->setProperty("appId", appId);
                r2->setProperty("destPath", destPath);
                r2->setProperty("step", QStringLiteral("asset"));
                pending_++;
                if (pending_ == 0)
                    emit allDownloadsFinished();
                return;
            }
        }
        emit downloadFinished(appId, false, destPath);
        if (pending_ == 0)
            emit allDownloadsFinished();
        return;
    }

    // Étape finale (asset GitHub ou téléchargement direct)
    bool success = false;
    QFile f(destPath);
    if (f.open(QIODevice::WriteOnly)) {
        f.write(data);
        f.close();
        success = true;
    }

    emit downloadFinished(appId, success, destPath);

    if (pending_ == 0)
        emit allDownloadsFinished();
}
