#include "Installer.hpp"
#include "CommandBuilder.hpp"
#include <QProcess>
#include <QTimer>

Installer::Installer(QObject* parent) : QObject(parent) {
    process_ = new QProcess(this);
    connect(process_, &QProcess::finished, this, &Installer::onProcessFinished);
}

void Installer::install(const QList<AppItem*>& items) {
    repairMode_ = false;
    queue_ = items;
    remaining_ = 0;
    for (auto* item : queue_) {
        if (!item->updateCommandForOS().isEmpty())
            remaining_++;
    }

    if (remaining_ == 0) {
        emit allInstalled();
        return;
    }
    startNext();
}

void Installer::repair(const QList<AppItem*>& items) {
    repairMode_ = true;
    queue_ = items;
    remaining_ = 0;
    for (auto* item : queue_) {
        if (!item->repairCommandForOS().isEmpty())
            remaining_++;
    }

    if (remaining_ == 0) {
        emit allInstalled();
        return;
    }
    startNext();
}

void Installer::startNext() {
    while (!queue_.isEmpty()) {
        AppItem* item = queue_.takeFirst();
        QString cmd = repairMode_ ? item->repairCommandForOS()
                                  : item->updateCommandForOS();
        if (cmd.isEmpty())
            continue;

        currentAppId_ = item->id;
        currentOutput_.clear();
        emit installStarted(currentAppId_);

        QString program;
        QStringList args;

        if (CommandBuilder::isWindows()) {
            // Élévation admin : demande de confirmation UAC
            // cmd /c "commande"
            program = QStringLiteral("cmd.exe");
            args << QStringLiteral("/c") << cmd;
        } else {
            // Linux : sudo si la commande n'est pas déjà sudoée
            if (cmd.startsWith("sudo"))
                program = QStringLiteral("bash");
            else
                program = QStringLiteral("sudo");
            args << QStringLiteral("-c") << cmd;
        }

        process_->start(program, args);
        return;
    }

    emit allInstalled();
}

void Installer::onProcessFinished() {
    currentOutput_ = QString::fromUtf8(process_->readAll()).trimmed();
    bool success = (process_->exitStatus() == QProcess::NormalExit &&
                    process_->exitCode() == 0);
    emit installFinished(currentAppId_, success, currentOutput_);
    remaining_--;

    if (remaining_ == 0)
        emit allInstalled();
    else
        startNext();
}
