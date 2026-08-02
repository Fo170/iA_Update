#include "CommandBuilder.hpp"
#include <QSysInfo>

CommandBuilder::CommandBuilder() {}

bool CommandBuilder::isWindows() {
    return QSysInfo::productType() == "windows";
}

QString CommandBuilder::scriptExtension() {
    return isWindows() ? QStringLiteral(".bat") : QStringLiteral(".sh");
}

QString CommandBuilder::buildScript(const QList<AppItem*>& items) const {
    if (isWindows()) {
        QString script = QStringLiteral("@echo off\r\n")
            + QStringLiteral("REM Script de mise a jour genere par iA_Update\r\n")
            + QStringLiteral("REM Executez ce fichier avec les droits administrateur si necessaire.\r\n\r\n");

        for (auto* item : items) {
            if (!item->updateCommandForOS().isEmpty()) {
                script += QStringLiteral("echo.\r\n")
                    + QStringLiteral("echo === %1 ===\r\n")
                        .arg(item->name)
                    + QStringLiteral("echo.\r\n")
                    + item->updateCommandForOS() + QStringLiteral("\r\n");
            }
        }

        script += QStringLiteral("\r\necho.\r\n")
            + QStringLiteral("echo Termine. Appuyez sur une touche pour fermer.\r\n")
            + QStringLiteral("pause >nul\r\n");
        return script;
    }

    QString script = QStringLiteral("#!/bin/bash\n")
        + QStringLiteral("# Script de mise a jour genere par iA_Update\n")
        + QStringLiteral("# Executez ce fichier avec les droits root si necessaire.\n\n")
        + QStringLiteral("set -e\n\n");

    for (auto* item : items) {
        if (!item->updateCommandForOS().isEmpty()) {
            script += QStringLiteral("echo \"=== %1 ===\"\n").arg(item->name)
                + item->updateCommandForOS() + QStringLiteral("\n\n");
        }
    }

    script += QStringLiteral("echo \"Termine.\"\n");
    return script;
}
