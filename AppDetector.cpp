#include "AppDetector.hpp"
#include <QProcess>
#include <QProcessEnvironment>
#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>
#include <QSysInfo>
#include <QSettings>
#include <QVersionNumber>

AppDetector::AppDetector() {}

bool AppDetector::isWindows() {
    return QSysInfo::productType() == "windows";
}

QString AppDetector::runProgram(const QString& program, const QStringList& args,
                                const QString& fallbackLine) const {
    QProcess proc;
    proc.setProcessChannelMode(QProcess::MergedChannels);

    if (!fallbackLine.isEmpty())
        proc.start(program, {QStringLiteral("/d"), QStringLiteral("/c"), fallbackLine});
    else
        proc.start(program, args);

    if (!proc.waitForStarted(4000))
        return QString();
    if (!proc.waitForFinished(8000))
        return QString();
    return QString::fromUtf8(proc.readAll()).trimmed();
}

QString AppDetector::runCommand(const QStringList& args) const {
    if (args.isEmpty())
        return QString();

    QString program = args.value(0);
    QStringList rest = args.mid(1);

    // Résout le chemin réel du programme (gère .exe et .cmd/.bat)
    QString resolved = findInPath(program);
    QString lower = resolved.toLower();

    if (isWindows()) {
        if (lower.endsWith(".cmd") || lower.endsWith(".bat") || resolved.isEmpty()) {
            // cmd.exe /d /c "prog args..." — ligne unique, quoting correct.
            // Les guillemets ne sont ajoutés que si l'élément contient des
            // espaces, sinon cmd casse la ligne.
            QStringList parts;
            parts << program;
            parts << rest;
            QStringList quoted;
            for (const QString& p : parts) {
                if (p.contains(' '))
                    quoted << ("\"" + p + "\"");
                else
                    quoted << p;
            }
            QString line = quoted.join(' ');
            return runProgram(QStringLiteral("cmd.exe"), QStringList(), line);
        }
        return runProgram(resolved, rest);
    }

    return runProgram(resolved.isEmpty() ? program : resolved, rest);
}

QString AppDetector::findInPath(const QString& cmd) const {
    if (cmd.isEmpty())
        return QString();

    QStringList args;
    if (isWindows())
        args = {QStringLiteral("where.exe"), cmd};
    else
        args = {QStringLiteral("which"), cmd};

    QString out = runProgram(args.value(0), args.mid(1));
    if (out.isEmpty())
        return QString();

    const QStringList lines = out.split(QRegularExpression("\\r?\\n"), Qt::SkipEmptyParts);

    // Collecte les chemins existants, en préférant .exe puis .cmd/.bat
    QString exe, cmdScript, fallback;
    for (const QString& line : lines) {
        QString p = line.trimmed();
        if (p.isEmpty() || p.startsWith("Information:", Qt::CaseInsensitive))
            continue;
        if (!QFileInfo(p).exists())
            continue;
        if (fallback.isEmpty())
            fallback = p;
        QString lower = p.toLower();
        if (lower.endsWith(".exe") && exe.isEmpty())
            exe = p;
        else if ((lower.endsWith(".cmd") || lower.endsWith(".bat")) && cmdScript.isEmpty())
            cmdScript = p;
    }
    if (!exe.isEmpty()) return exe;
    if (!cmdScript.isEmpty()) return cmdScript;
    return fallback;
}

QString AppDetector::findInPaths(const QStringList& paths) const {
    for (const QString& p : paths) {
        QString expanded = p;
        if (QFileInfo::exists(expanded))
            return QDir::toNativeSeparators(QFileInfo(expanded).absoluteFilePath());

        if (expanded.contains('*')) {
            // Développe le premier segment wildcard (ex: C:/ghostwriter_*/ghostwriter.exe)
            QString result = expandFirstWildcard(expanded);
            if (!result.isEmpty() && QFileInfo::exists(result))
                return QDir::toNativeSeparators(QFileInfo(result).absoluteFilePath());
        }
    }
    return QString();
}

// Développe le premier segment contenant '*' d'un chemin :
//   "C:/foo_*/bar.exe" -> "C:/foo_1.2.3/bar.exe" (premier dossier existant)
// Retourne une chaîne vide si aucune correspondance.
QString AppDetector::expandFirstWildcard(const QString& path) const {
    int starIdx = path.indexOf('*');
    if (starIdx < 0)
        return path;

    int segStart = path.lastIndexOf('/', starIdx);
    int segEnd = path.indexOf('/', starIdx);
    if (segEnd < 0)
        segEnd = path.length();

    QString baseDir = path.left(segStart + 1);          // "C:/"
    QString pattern = path.mid(segStart + 1, segEnd - segStart - 1); // "foo_*"
    QString suffix = path.mid(segEnd);                  // "/bar.exe"

    QDir d(baseDir);
    QStringList patterns = {pattern};
    QStringList entries = d.entryList(patterns,
                                      QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot,
                                      QDir::Name);
    if (entries.isEmpty())
        return QString();

    QString candidate = baseDir + entries.first() + suffix;
    // Développe récursivement les éventuels wildcards restants
    if (candidate.contains('*'))
        return expandFirstWildcard(candidate);
    return candidate;
}

QString AppDetector::extractVersion(const QString& output, const QString& regex) const {
    if (regex.isEmpty() || output.isEmpty())
        return QString();
    QRegularExpression re(regex);
    QRegularExpressionMatch m = re.match(output);
    if (m.hasMatch())
        return m.captured(1);
    return QString();
}

QString AppDetector::registryVersion(const QString& displayNamePattern) const {
    if (!isWindows() || displayNamePattern.isEmpty())
        return QString();

    const QStringList roots = {
        QStringLiteral("HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall"),
        QStringLiteral("HKEY_LOCAL_MACHINE\\SOFTWARE\\WOW6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall"),
        QStringLiteral("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall")
    };

    QString best;
    QVersionNumber bestVer;

    for (const QString& root : roots) {
        QSettings reg(root, QSettings::NativeFormat);
        const QStringList groups = reg.childGroups();
        for (const QString& g : groups) {
            reg.beginGroup(g);
            QString name = reg.value("DisplayName").toString();
            QString version = reg.value("DisplayVersion").toString();
            QString installLoc = reg.value("InstallLocation").toString();
            reg.endGroup();

            if (name.contains(displayNamePattern, Qt::CaseInsensitive) && !version.isEmpty()) {
                // Version brute souvent "1.2.3.4" — normalisée en X.Y.Z
                QString v = version;
                QRegularExpression re("(\\d+\\.\\d+\\.\\d+)");
                QRegularExpressionMatch m = re.match(v);
                if (m.hasMatch())
                    v = m.captured(1);

                QVersionNumber qv = QVersionNumber::fromString(v);
                if (!qv.isNull() && (bestVer.isNull() || qv > bestVer)) {
                    bestVer = qv;
                    best = v;
                } else if (best.isEmpty()) {
                    best = v;
                }
            }
        }
    }
    return best;
}

AppScope AppDetector::inferScope(const AppItem& item) const {
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

    QStringList cmds = item.detectCommands();        // commandes version
    QStringList locCmds = item.detectLocateCommand(); // commandes localisation PATH
    QStringList paths = item.detectPaths();

    QStringList searchCmds = !locCmds.isEmpty() ? locCmds : cmds;

    // 1. Détection via commande dans le PATH
    if (!searchCmds.isEmpty()) {
        QString found = findInPath(searchCmds.first());
        if (!found.isEmpty()) {
            item.installed = true;
            item.referenced_in_path = true;
            item.install_path = QDir::toNativeSeparators(QFileInfo(found).absolutePath());

            if (!cmds.isEmpty()) {
                QString out = runCommand(cmds);
                if (!out.isEmpty())
                    item.local_version = extractVersion(out, item.versionLocalRegex);
            }
        }
    }

    // 2. Détection via chemins d'installation (si pas déjà trouvé)
    if (!item.installed && !paths.isEmpty()) {
        QString found = findInPaths(paths);
        if (!found.isEmpty()) {
            item.installed = true;
            item.install_path = found;
            // Un binaire trouvé dans un chemin connu peut être dans le PATH
            // même si la commande n'a pas été trouvée directement.
            if (!searchCmds.isEmpty() && !findInPath(searchCmds.first()).isEmpty())
                item.referenced_in_path = true;
        }
    }

    // 3. Lecture version locale pour les binaires trouvés via chemins
    if (item.installed && item.local_version.isEmpty() && !cmds.isEmpty()) {
        QString out = runCommand(cmds);
        if (!out.isEmpty())
            item.local_version = extractVersion(out, item.versionLocalRegex);
    }

    // 4. Fallback : version depuis le registre Windows
    if (item.installed && item.local_version.isEmpty() && !item.versionLocalRegistry.isEmpty())
        item.local_version = registryVersion(item.versionLocalRegistry);

    // 5. Fallback : version depuis le chemin d'installation (ex: ghostwriter_2.1.6_win64_portable)
    if (item.installed && item.local_version.isEmpty() && !item.versionLocalFromPath.isEmpty())
        item.local_version = extractVersion(item.install_path, item.versionLocalFromPath);

    if (item.installed)
        item.scope = inferScope(item);
    else
        item.scope = AppScope::Unknown;

    item.computeStatus();
}
