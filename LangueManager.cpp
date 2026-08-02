#include "LangueManager.hpp"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QLocale>

const QHash<QString, QString> LangueManager::displayNames_ = {
    {"francais", "Français"},
    {"anglais",  "English"}
};

LangueManager::LangueManager(const QString& langDir, QObject* parent)
    : QObject(parent), langDir_(langDir) {
    network_ = new QNetworkAccessManager(this);
}

bool LangueManager::load(const QString& languageCode) {
    QString path = langDir_ + "/" + languageCode + ".txt";
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return false;

    QHash<QString, QString> newTranslations;
    QTextStream in(&f);
    in.setEncoding(QStringConverter::Utf8);

    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();

        if (line.isEmpty() || line.startsWith('#'))
            continue;

        int sep = line.indexOf('=');
        if (sep <= 0)
            continue;

        QString key = line.left(sep).trimmed();
        QString value = line.mid(sep + 1).trimmed();
        newTranslations[key] = value;
    }

    f.close();

    if (newTranslations.isEmpty())
        return false;

    translations_ = newTranslations;
    currentLang_ = languageCode;
    emit languageChanged();
    return true;
}

QString LangueManager::get(const QString& key) const {
    return translations_.value(key, key);
}

QString LangueManager::get(const QString& key, const QString& fallback) const {
    return translations_.value(key, fallback);
}

QString LangueManager::currentLanguage() const {
    return currentLang_;
}

QStringList LangueManager::availableLanguages() const {
    QDir d(langDir_);
    QStringList files = d.entryList({"*.txt"}, QDir::Files, QDir::Name);

    QStringList langs;
    for (const QString& f : files)
        langs.append(QFileInfo(f).completeBaseName());
    return langs;
}

QStringList LangueManager::languageDisplayNames() const {
    QStringList names;
    for (const QString& code : availableLanguages())
        names.append(displayNames_.value(code, code));
    return names;
}

QString LangueManager::detectSystemLanguage() {
    QString lang = QLocale::system().name().toLower();

    if (lang.startsWith("fr"))
        return "francais";
    if (lang.startsWith("en"))
        return "anglais";

    return "anglais";
}

void LangueManager::downloadLanguage(const QString& code, const QString& remoteUrl) {
    QNetworkReply* reply = network_->get(QNetworkRequest(QUrl(remoteUrl)));
    connect(reply, &QNetworkReply::finished, this, [this, reply, code]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            emit languageDownloaded(code, false);
            return;
        }
        QDir().mkpath(langDir_);
        QString path = langDir_ + "/" + code + ".txt";
        QFile f(path);
        if (!f.open(QIODevice::WriteOnly)) {
            emit languageDownloaded(code, false);
            return;
        }
        f.write(reply->readAll());
        f.close();
        emit languageDownloaded(code, true);
    });
}
