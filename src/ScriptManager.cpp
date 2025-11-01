#include "ScriptManager.h"
#include <QDir>
#include <QFileInfoList>
#include <QDebug>

QString ScriptManager::userScriptsDir() const {
    QString configuredDir = settings.tailScriptFilesSavePath();
    if (configuredDir.isEmpty()) {
        return {};
    }
    return configuredDir;
}

QStringList ScriptManager::getDefinedScripts() const {
    QStringList scripts{};
    const QString dirPath = userScriptsDir();
    const QDir dir(dirPath);
    if (!dir.exists()) {
        // No scripts directory, return empty list
        return scripts;
    }

    QFileInfoList files = dir.entryInfoList(QStringList() << "*.sh", QDir::Files);
    for (const QFileInfo& file : files) {
        if (!file.isExecutable()) {
            qWarning() << "Script file is not executable: " << file.absoluteFilePath() << ", skipping file.";
            continue;
        }
        scripts << file.absoluteFilePath();
    }

    return scripts;
}
