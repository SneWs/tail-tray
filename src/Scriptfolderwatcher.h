#ifndef SCRIPTFOLDERWATCHER_H
#define SCRIPTFOLDERWATCHER_H
#include <QFileSystemWatcher>
#include <QObject>

class ScriptFolderWatcher : public QObject
{
    Q_OBJECT
public:
    explicit ScriptFolderWatcher(QObject* parent = nullptr);

    void startWatching(const QString& path);

    void stopWatching();

signals:
    void scriptsChanged();

private slots:
    void onDirectoryChanged(const QString& path);

private:
    QFileSystemWatcher watcher;
};

#endif // SCRIPTFOLDERWATCHER_H
