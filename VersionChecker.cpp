#include "VersionChecker.hpp"
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrl>
#include <QUrlQuery>
#include "AppConfig.hpp"

VersionChecker::VersionChecker(QObject* parent)
    : QObject(parent) {
    manager_ = new QNetworkAccessManager(this);
    connect(manager_, &QNetworkAccessManager::finished,
            this, &VersionChecker::onReplyFinished);
}

bool VersionChecker::isChecking() const {
    return checking_;
}

QUrl VersionChecker::buildUrl(const AppItem& item) const {
    QString type = item.online.value("type").toString();
    QString key = item.online.value("key").toString();
    QString repo = item.online.value("repo").toString();
    QString pkg = item.online.value("package").toString();

    if (type == "github") {
        return QUrl(QStringLiteral("https://api.github.com/repos/%1/releases/latest").arg(repo));
    }
    if (type == "npm") {
        // L'API npm expose une liste de versions ; on interroge l'endpoint dist-tags
        return QUrl(QStringLiteral("https://registry.npmjs.org/%1/latest").arg(pkg));
    }
    if (type == "pypi") {
        return QUrl(QStringLiteral("https://pypi.org/pypi/%1/json").arg(pkg));
    }
    if (type == "endoflife") {
        return QUrl(QStringLiteral("https://endoflife.date/api/%1.json").arg(key));
    }
    return QUrl();
}

QString VersionChecker::parseVersion(const AppItem& item, const QByteArray& data) const {
    QString type = item.online.value("type").toString();

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(data, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject())
        return QString();

    QJsonObject obj = doc.object();

    if (type == "github") {
        // tag_name: "v1.2.3" ou "1.2.3"
        QString tag = obj.value("tag_name").toString();
        return tag.remove(0, 1); // retire le 'v' préfixe
    }
    if (type == "npm") {
        // {"version":"1.2.3", ...}
        return obj.value("version").toString();
    }
    if (type == "pypi") {
        return obj.value("info").toObject().value("version").toString();
    }
    if (type == "endoflife") {
        // L'API endoflife.date renvoie un tableau [{cycle, latest, ...}, ...]
        // On prend le premier cycle (le plus récent) et son champ "latest".
        QJsonArray arr = doc.array();
        if (!arr.isEmpty())
            return arr.first().toObject().value("latest").toString();
        return QString();
    }
    return QString();
}

void VersionChecker::checkAll(QList<AppItem>& items) {
    if (checking_) return;
    checking_ = true;
    running_ = 0;

    pending_.clear();
    for (auto& item : items) {
        item.online_checked = false;
        item.online_error = false;
        item.online_version.clear();

        QString type = item.online.value("type").toString();
        if (type == "none" || type.isEmpty()) {
            // Pas de source en ligne : statut "inconnue en ligne" si installé
            item.online_checked = true;
            item.computeStatus();
            continue;
        }

        QUrl url = buildUrl(item);
        if (url.isEmpty()) {
            item.online_checked = true;
            item.online_error = true;
            item.computeStatus();
            continue;
        }

        QNetworkRequest request(url);
        request.setHeader(QNetworkRequest::UserAgentHeader,
            QStringLiteral(APP_NAME "/%1").arg(QStringLiteral(APP_VERSION)));
        request.setTransferTimeout(15000);
        request.setRawHeader("Accept", "application/vnd.github+json");

        // On stocke l'AppItem à mettre à jour dans les propriétés du reply
        pending_.append(&item);
        QNetworkReply* reply = manager_->get(request);
        reply->setProperty("appId", item.id);
        running_++;
    }

    if (running_ == 0) {
        checking_ = false;
        emit allChecked();
    }
}

void VersionChecker::onReplyFinished(QNetworkReply* reply) {
    reply->deleteLater();
    running_--;

    QString appId = reply->property("appId").toString();

    // Retrouve l'AppItem correspondant
    AppItem* item = nullptr;
    for (auto* p : pending_) {
        if (p->id == appId) {
            item = p;
            break;
        }
    }

    if (item) {
        item->online_checked = true;
        if (reply->error() != QNetworkReply::NoError) {
            item->online_error = true;
            item->error_message = reply->errorString();
            emit checkError(appId, reply->errorString());
        } else {
            QString version = parseVersion(*item, reply->readAll());
            if (version.isEmpty()) {
                item->online_error = true;
                item->error_message = QStringLiteral("Version introuvable dans la réponse");
            } else {
                item->online_version = version;
            }
        }
        item->computeStatus();
        emit appChecked(appId);
    }

    if (running_ == 0) {
        pending_.clear();
        checking_ = false;
        emit allChecked();
    }
}
