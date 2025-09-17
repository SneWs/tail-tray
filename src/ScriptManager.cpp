#include "ScriptManager.h"
#include <QStandardPaths>
#include <QDir>
#include <QFileInfoList>
#include <QDebug>

QString ScriptManager::userScriptsDir() {
    QString baseDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    return baseDir + "/scripts";
}

void ScriptManager::ensureScriptsDirExist() {
    QDir dir(userScriptsDir());

    if (!dir.exists()) {
        if(dir.mkpath(".")) {
            qDebug() << "Path created";
        } else {
            qDebug() << "Path could not be created";
        }
    }
}

QStringList ScriptManager::listScripts() {
    QDir dir(userScriptsDir());


    QStringList filters;
    filters << "*.sh";

    QStringList scripts;
    QFileInfoList files = dir.entryInfoList(filters, QDir::Files);
    for (const QFileInfo &file : files) {
        scripts << file.absoluteFilePath();
    }

    return scripts;
}
