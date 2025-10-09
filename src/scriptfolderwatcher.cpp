#include "scriptfolderwatcher.h"
#include <QDebug>
#include <QDir>

ScriptFolderWatcher::ScriptFolderWatcher(QObject* parent) : QObject(parent)
{
    connect(&watcher, &QFileSystemWatcher::directoryChanged,
            this, &ScriptFolderWatcher::onDirectoryChanged);
}

void ScriptFolderWatcher::startWatching(const QString& path) {
    if (path.isEmpty()) {
        return;
    }

    watcher.removePaths(watcher.directories());

    if (!watcher.addPath(path)) {
        qWarning() << "Failed to watch directory: " << path;
    } else {
        qDebug() << "Watching script directory: " << path;
    }

    emit scriptsChanged();
}

void ScriptFolderWatcher::onDirectoryChanged(const QString& path) {
    qDebug() << "Script folder changed:" << path;

    emit scriptsChanged();
}
