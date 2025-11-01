#ifndef SCRIPTMANAGER_H
#define SCRIPTMANAGER_H

#include <memory>
#include <QFileSystemWatcher>

#include "TailSettings.h"

#include <QString>
#include <QStringList>

class ScriptManager : public QObject
{
    Q_OBJECT
public:
    explicit ScriptManager(TailSettings& s, QObject* parent = nullptr);

    [[nodiscard]] QString userScriptsDir() const;
    [[nodiscard]] QStringList getDefinedScripts() const;

    void tryInstallWatcher();

signals:
    void availableScriptsChanged();

private slots:
    void onDirectoryChanged(const QString &path);

private:
    TailSettings& settings;
    std::unique_ptr<QFileSystemWatcher> pWatcher;
};

#endif // SCRIPTMANAGER_H
