#include "AppItem.hpp"
#include <QJsonDocument>
#include <QJsonArray>
#include <QFile>
#include <QProcessEnvironment>
#include <QVersionNumber>

AppItem::AppItem() {}

QList<AppItem> AppItem::loadManifest(const QString& path, QString* error) {
    QList<AppItem> items;
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        if (error) *error = QStringLiteral("Impossible d'ouvrir %1").arg(path);
        return items;
    }

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &parseError);
    f.close();

    if (parseError.error != QJsonParseError::NoError) {
        if (error) *error = QStringLiteral("JSON invalide : %1").arg(parseError.errorString());
        return items;
    }
    if (!doc.isObject()) {
        if (error) *error = QStringLiteral("Structure JSON invalide");
        return items;
    }

    QJsonArray apps = doc.object().value("apps").toArray();
    for (const auto& v : apps) {
        if (v.isObject())
            items.append(fromJson(v.toObject()));
    }
    return items;
}

AppItem AppItem::fromJson(const QJsonObject& obj) {
    AppItem item;
    item.id = obj.value("id").toString();
    item.name = obj.value("name").toString();
    item.category = obj.value("category").toString();
    item.homepage = obj.value("homepage").toString();
    item.scope_default = obj.value("scope_default").toString("global");
    item.downloadUrl = obj.value("downloadUrl").toString();
    item.detect = obj.value("detect").toObject();
    item.online = obj.value("online").toObject();
    item.updateCommand = obj.value("updateCommand").toObject();
    item.versionLocalRegex = obj.value("versionLocal").toObject().value("regex").toString();
    return item;
}

QJsonObject AppItem::toJson() const {
    QJsonObject obj;
    obj["id"] = id;
    obj["name"] = name;
    obj["category"] = category;
    obj["homepage"] = homepage;
    obj["scope_default"] = scope_default;
    obj["downloadUrl"] = downloadUrl;
    obj["detect"] = detect;
    obj["online"] = online;
    obj["updateCommand"] = updateCommand;
    QJsonObject v;
    v["regex"] = versionLocalRegex;
    obj["versionLocal"] = v;
    return obj;
}

QStringList AppItem::detectCommands() const {
    QStringList out;
    QString key = QSysInfo::productType() == "windows" ? "windows" : "linux";
    QJsonValue cmdVal = detect.value(key).toObject().value("cmd");
    if (cmdVal.isArray()) {
        for (const auto& v : cmdVal.toArray())
            out << v.toString();
    } else if (cmdVal.isString()) {
        QString s = cmdVal.toString();
        if (!s.isEmpty())
            out << s;
    }
    return out;
}

QStringList AppItem::detectPaths() const {
    QStringList out;
    QString key = QSysInfo::productType() == "windows" ? "windows" : "linux";
    QJsonValue pVal = detect.value(key).toObject().value("paths");
    if (pVal.isArray()) {
        for (const auto& v : pVal.toArray()) {
            QString p = v.toString();
            if (!p.isEmpty()) {
                // Résout les variables d'environnement (%VAR% sur Windows, $VAR sur Linux)
                QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
                p.replace("%LOCALAPPDATA%", env.value("LOCALAPPDATA"));
                p.replace("%APPDATA%", env.value("APPDATA"));
                p.replace("%USERPROFILE%", env.value("USERPROFILE"));
                p.replace("$HOME", env.value("HOME"));
                out << p;
            }
        }
    }
    return out;
}

QString AppItem::updateCommandForOS() const {
    QString key = QSysInfo::productType() == "windows" ? "windows" : "linux";
    return updateCommand.value(key).toString();
}

void AppItem::computeStatus() {
    if (!installed) {
        status = AppStatus::NotInstalled;
        return;
    }
    if (!online_checked) {
        status = local_version.isEmpty() ? AppStatus::UnknownLocal : AppStatus::UpToDate;
        return;
    }
    if (online_error || online_version.isEmpty()) {
        status = AppStatus::UnknownOnline;
        return;
    }

    if (local_version.isEmpty()) {
        status = AppStatus::UnknownLocal;
        return;
    }

    QVersionNumber local = QVersionNumber::fromString(local_version);
    QVersionNumber online = QVersionNumber::fromString(online_version);

    if (local.isNull() || online.isNull()) {
        // Comparaison impossible : on tombe sur la comparaison texte simple
        status = (local_version == online_version) ? AppStatus::UpToDate
                                                   : AppStatus::UpdateAvailable;
        return;
    }

    status = (online > local) ? AppStatus::UpdateAvailable : AppStatus::UpToDate;
}

QString AppItem::statusKey() const {
    switch (status) {
        case AppStatus::NotInstalled:     return "status.not_installed";
        case AppStatus::UpToDate:         return "status.up_to_date";
        case AppStatus::UpdateAvailable:  return "status.update_available";
        case AppStatus::UnknownLocal:     return "status.unknown_local";
        case AppStatus::UnknownOnline:    return "status.unknown_online";
        case AppStatus::Error:            return "status.error";
    }
    return "status.error";
}

QString AppItem::scopeKey() const {
    switch (scope) {
        case AppScope::Global:  return "scope.global";
        case AppScope::User:    return "scope.user";
        case AppScope::Unknown: return "scope.unknown";
    }
    return "scope.unknown";
}
