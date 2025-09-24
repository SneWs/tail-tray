#include "ScriptManager.h"
#include <QStandardPaths>
#include <QDir>
#include <QFileInfoList>
#include <QDebug>

QString ScriptManager::userScriptsDir() {
    QString configuredDir = settings.tailScriptFilesSavePath();
    if (configuredDir.isEmpty()) {
        return QString();
    }
    else {
        return configuredDir;
    }
}

QStringList ScriptManager::getDefinedScripts() {
    QStringList scripts;
    QString dirPath = userScriptsDir();
    QDir dir(dirPath);
    if (!dir.exists()) {
        // No scripts directory, return empty list
        return scripts;
    }

    QFileInfoList files = dir.entryInfoList(QStringList() << "*.sh", QDir::Files);
    for (const QFileInfo& file : files) {
        scripts << file.absoluteFilePath();
    }

    return scripts;
}
