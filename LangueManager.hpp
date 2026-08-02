#ifndef LANGUEMANAGER_HPP
#define LANGUEMANAGER_HPP

#include <QObject>
#include <QString>
#include <QHash>
#include <QStringList>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>

class LangueManager : public QObject {
    Q_OBJECT
public:
    explicit LangueManager(const QString& langDir, QObject* parent = nullptr);

    bool load(const QString& languageCode);
    QString get(const QString& key) const;
    QString get(const QString& key, const QString& fallback) const;
    QString currentLanguage() const;
    QStringList availableLanguages() const;
    QStringList languageDisplayNames() const;
    static QString detectSystemLanguage();
    void downloadLanguage(const QString& code, const QString& remoteUrl);

signals:
    void languageChanged();
    void languageDownloaded(const QString& code, bool success);

private:
    QHash<QString, QString> translations_;
    QString langDir_;
    QString currentLang_;
    QNetworkAccessManager* network_;
    static const QHash<QString, QString> displayNames_;
};

#endif
