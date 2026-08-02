#include "AppDetector.hpp"
#include <QProcess>
#include <QProcessEnvironment>
#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>
#include <QSysInfo>
#include <QStandardPaths>

AppDetector::AppDetector() {}

bool AppDetector::isWindows() {
    return QSysInfo::productType() == "windows";
}

QString AppDetector::runCommand(const QStringList& args) const {
    QProcess proc;
    proc.setProcessChannelMode(QProcess::MergedChannels);
    proc.start(args.value(0), args.mid(1));
    if (!proc.waitForStarted(3000))
        return QString();
    if (!proc.waitForFinished(5000))
        return QString();
    return QString::fromUtf8(proc.readAll()).trimmed();
}

QString AppDetector::findInPath(const QString& cmd) const {
    if (cmd.isEmpty())
        return QString();

    // Windows : où se trouve le binaire ? via where.exe
    // Linux : via which
    QStringList args;
    if (isWindows())
        args = {QStringLiteral("where.exe"), cmd};
    else
        args = {QStringLiteral("which"), cmd};

    QString out = runCommand(args);
    if (out.isEmpty())
        return QString();

    // where peut retourner plusieurs lignes : on garde la première
    QString first = out.split(QRegularExpression("\\r?\\n"), Qt::SkipEmptyParts).value(0);
    return first.trimmed();
}

QString AppDetector::findInPaths(const QStringList& paths) const {
    for (const QString& p : paths) {
        QString expanded = p;
        if (QFileInfo::exists(expanded))
            return QDir::toNativeSeparators(QFileInfo(expanded).absoluteFilePath());

        // Support des wildcards simples (*) dans les chemins (ex: C:/Qt/*)
        if (expanded.contains('*')) {
            QString base = expanded.left(expanded.indexOf('*'));
            QDir d(base);
            if (d.exists()) {
                QStringList entries = d.entryList(QDir::Dirs | QDir::Files);
                if (!entries.isEmpty())
                    return QDir::toNativeSeparators(d.absoluteFilePath(entries.first()));
            }
        }
    }
    return QString();
}

QString AppDetector::extractVersion(const QString& output, const QString& regex) const {
    if (regex.isEmpty())
        return QString();
    QRegularExpression re(regex);
    QRegularExpressionMatch m = re.match(output);
    if (m.hasMatch())
        return m.captured(1);
    return QString();
}

AppScope AppDetector::inferScope(const AppItem& item) const {
    // Portée basée sur le chemin : LocalAppData / AppData => utilisateur
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    QString local = env.value("LOCALAPPDATA").toLower();
    QString appdata = env.value("APPDATA").toLower();

    QString path = item.install_path.toLower();
    if ((!local.isEmpty() && path.startsWith(local)) ||
        (!appdata.isEmpty() && path.startsWith(appdata)) ||
        (!item.install_path.isEmpty() && path.contains("appdata")))
        return AppScope::User;

    if (item.scope_default == "user")
        return AppScope::User;
    if (item.scope_default == "global")
        return AppScope::Global;

    return AppScope::Unknown;
}

void AppDetector::detect(AppItem& item) const {
    item.installed = false;
    item.install_path.clear();
    item.local_version.clear();
    item.referenced_in_path = false;
    item.error_message.clear();

    QStringList cmds = item.detectCommands();
    QStringList paths = item.detectPaths();

    // 1. Détection via commande dans le PATH
    if (!cmds.isEmpty()) {
        QString found = findInPath(cmds.first());
        if (!found.isEmpty()) {
            item.installed = true;
            item.referenced_in_path = true;
            item.install_path = QDir::toNativeSeparators(QFileInfo(found).absolutePath());

            // Lecture de la version via la commande complète (ex: python --version)
            QString out = runCommand(cmds);
            if (!out.isEmpty()) {
                item.local_version = extractVersion(out, item.versionLocalRegex);
                if (item.local_version.isEmpty() && !item.versionLocalRegex.isEmpty()) {
                    // Le --version peut écrire sur stderr (python) — déjà fusionné
                    // Réessai avec le nom seul pour certains binaires
                }
            }
        }
    }

    // 2. Détection via chemins d'installation
    if (!item.installed && !paths.isEmpty()) {
        QString found = findInPaths(paths);
        if (!found.isEmpty()) {
            item.installed = true;
            item.install_path = found;
        }
    }

    // 3. Lecture version locale pour les binaires trouvés via chemins
    if (item.installed && item.local_version.isEmpty() && !cmds.isEmpty()) {
        QString out = runCommand(cmds);
        if (!out.isEmpty())
            item.local_version = extractVersion(out, item.versionLocalRegex);
    }

    if (item.installed)
        item.scope = inferScope(item);
    else
        item.scope = AppScope::Unknown;

    item.computeStatus();
}
