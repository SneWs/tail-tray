#include "ScriptManager.h"
#include <QStandardPaths>
#include <QDir>
#include <QFileInfoList>
#include <QDebug>

QString ScriptManager::userScriptsDir() {
    QString configuredDir = settings.tailScriptFilesSavePath();
    if (configuredDir.isEmpty()) {
        return QString();
    } else {
        return configuredDir;
    }
}

QStringList ScriptManager::listScripts() {
    QString dirPath = userScriptsDir();

    if (dirPath.isEmpty()) {
        return {};
    }

    QDir dir(dirPath);


    QStringList filters;
    filters << "*.sh";

    QStringList scripts;
    QFileInfoList files = dir.entryInfoList(filters, QDir::Files);
    for (const QFileInfo &file : files) {
        scripts << file.absoluteFilePath();
    }

    return scripts;
}
