#include "ScriptManager.h"
#include <QDir>
#include <QFileInfoList>
#include <QDebug>
#include <QFileSystemWatcher>

ScriptManager::ScriptManager(TailSettings &s, QObject* parent)
    : QObject(parent)
    , settings(s)
{ }

QString ScriptManager::userScriptsDir() const {
    QString configuredDir = settings.tailScriptFilesSavePath();
    if (configuredDir.isEmpty()) {
        return {};
    }
    return configuredDir;
}

QStringList ScriptManager::getDefinedScripts() const {
    QStringList scripts{};
    const QDir dir(userScriptsDir());
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

void ScriptManager::tryInstallWatcher() {
    if (pWatcher) {
        pWatcher.reset(nullptr);
    }

    const QDir dir(userScriptsDir());
    if (!dir.exists())
        return;

    pWatcher = std::make_unique<QFileSystemWatcher>(this);
    connect(pWatcher.get(), &QFileSystemWatcher::directoryChanged,
            this,    &ScriptManager::onDirectoryChanged);

    pWatcher->addPath(dir.absolutePath());
}

void ScriptManager::onDirectoryChanged(const QString&) {
    emit availableScriptsChanged();
}
