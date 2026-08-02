#include "VersionChecker.hpp"
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QDir>
#include <QUrl>
#include <QUrlQuery>
#include <QRegularExpression>
#include <QCoreApplication>
#include "AppConfig.hpp"

VersionChecker::VersionChecker(QObject* parent)
    : QObject(parent) {
    manager_ = new QNetworkAccessManager(this);
    connect(manager_, &QNetworkAccessManager::finished,
            this, &VersionChecker::onReplyFinished);
    loadCache();
}

bool VersionChecker::isChecking() const {
    return checking_;
}

void VersionChecker::loadCache() {
    QFile f(QCoreApplication::applicationDirPath() + "/cache_online.json");
    if (!f.open(QIODevice::ReadOnly))
        return;
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &err);
    f.close();
    if (err.error != QJsonParseError::NoError || !doc.isObject())
        return;
    QJsonObject obj = doc.object();
    for (auto it = obj.begin(); it != obj.end(); ++it) {
        QJsonObject entry = it.value().toObject();
        cacheVersion_[it.key()] = entry.value("version").toString();
        cacheTime_[it.key()] = static_cast<qint64>(entry.value("ts").toDouble());
    }
}

void VersionChecker::saveCache() const {
    QJsonObject obj;
    for (auto it = cacheVersion_.begin(); it != cacheVersion_.end(); ++it) {
        QJsonObject entry;
        entry["version"] = it.value();
        entry["ts"] = static_cast<double>(cacheTime_.value(it.key()));
        obj[it.key()] = entry;
    }
    QFile f(QCoreApplication::applicationDirPath() + "/cache_online.json");
    if (f.open(QIODevice::WriteOnly)) {
        f.write(QJsonDocument(obj).toJson(QJsonDocument::Indented));
        f.close();
    }
}

void VersionChecker::forceRefresh(QList<AppItem>& items) {
    force_ = true;
    checkAll(items, 0);
    force_ = false;
}

QString VersionChecker::githubAtomUrl(const AppItem& item) {
    if (item.online.value("type").toString() != "github")
        return QString();
    QString repo = item.online.value("repo").toString();
    if (repo.isEmpty())
        return QString();
    return QStringLiteral("https://github.com/%1/releases.atom").arg(repo);
}

QUrl VersionChecker::buildUrl(const AppItem& item) const {
    QString type = item.online.value("type").toString();
    QString key = item.online.value("key").toString();
    QString repo = item.online.value("repo").toString();
    QString pkg = item.online.value("package").toString();

    if (type == "github") {
        if (item.online.value("useTags").toBool())
            return QUrl(QStringLiteral("https://api.github.com/repos/%1/tags").arg(repo));
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
    if (err.error != QJsonParseError::NoError)
        return QString();

    // Les endpoints github /tags et endoflife renvoient un tableau JSON.
    bool isArray = doc.isArray();
    if (!isArray && !doc.isObject())
        return QString();

    QJsonObject obj = doc.object();

    if (type == "github") {
        // tag_name: "v1.2.3" ou "1.2.3"
        QString tag = obj.value("tag_name").toString();
        if (tag.isEmpty()) {
            // Endpoint /tags : tableau d'objets [{name: "v1.2.3", ...}, ...]
            QJsonArray arr = doc.array();
            if (!arr.isEmpty())
                tag = arr.first().toObject().value("name").toString();
        }
        // Retire le préfixe 'v' s'il existe (ex: "v1.2.3" -> "1.2.3"),
        // mais conserve le tag tel quel sinon (ex: "2.7.3").
        if (tag.startsWith('v', Qt::CaseInsensitive))
            tag.remove(0, 1);
        return tag;
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

void VersionChecker::checkAll(QList<AppItem>& items, int cacheAgeH) {
    if (checking_) return;
    checking_ = true;
    running_ = 0;

    qint64 now = QDateTime::currentSecsSinceEpoch();
    qint64 maxAge = static_cast<qint64>(cacheAgeH) * 3600;

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

        // Cache : version connue et encore fraîche ?
        bool cacheOk = cacheVersion_.contains(item.id) &&
                       (force_ || (now - cacheTime_.value(item.id)) < maxAge);
        if (cacheOk && !cacheVersion_.value(item.id).isEmpty()) {
            item.online_version = cacheVersion_.value(item.id);
            item.online_checked = true;
            item.computeStatus();
            emit appChecked(item.id);
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
        if (reply->error() != QNetworkReply::NoError) {
            // Fallback GitHub : si l'API est limitée (403), on retente via le
            // flux atom (releases.atom) qui n'utilise pas le quota API.
            if (item->online.value("type").toString() == "github" &&
                !reply->property("retriedAtom").toBool()) {
                QString atom = githubAtomUrl(*item);
                if (!atom.isEmpty()) {
                    QNetworkRequest req{QUrl(atom)};
                    req.setHeader(QNetworkRequest::UserAgentHeader,
                        QStringLiteral(APP_NAME "/%1").arg(QStringLiteral(APP_VERSION)));
                    req.setTransferTimeout(15000);
                    QNetworkReply* r2 = manager_->get(req);
                    r2->setProperty("appId", item->id);
                    r2->setProperty("retriedAtom", true);
                    running_++;
                    emit checkError(appId, reply->errorString());
                } else {
                    item->online_checked = true;
                    item->online_error = true;
                    item->error_message = reply->errorString();
                    item->computeStatus();
                    emit appChecked(appId);
                }
            } else {
                item->online_checked = true;
                item->online_error = true;
                item->error_message = reply->errorString();
                item->computeStatus();
                emit appChecked(appId);
            }
        } else {
            QString version;
            if (reply->property("retriedAtom").toBool()) {
                // Réponse atom XML : extraire la version du premier <title>
                QByteArray data = reply->readAll();
                QRegularExpression re("<title>\\s*v?([0-9]+\\.[0-9]+\\.[0-9]+[^<]*)</title>");
                QRegularExpressionMatch m = re.match(QString::fromUtf8(data));
                if (m.hasMatch()) {
                    version = m.captured(1).trimmed();
                    // retire le suffixe éventuel (ex: "v0.32.5" -> "0.32.5")
                    if (version.startsWith('v', Qt::CaseInsensitive))
                        version.remove(0, 1);
                }
            } else {
                version = parseVersion(*item, reply->readAll());
            }
            if (version.isEmpty()) {
                item->online_error = true;
                item->error_message = QStringLiteral("Version introuvable dans la réponse");
            } else {
                item->online_version = version;
                // Mise en cache pour éviter d'épuiser le quota d'API
                cacheVersion_[appId] = version;
                cacheTime_[appId] = QDateTime::currentSecsSinceEpoch();
            }
            item->computeStatus();
            emit appChecked(appId);
        }
    }

    if (running_ == 0) {
        pending_.clear();
        checking_ = false;
        saveCache();
        emit allChecked();
    }
}
