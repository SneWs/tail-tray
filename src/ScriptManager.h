#ifndef SCRIPTMANAGER_H
#define SCRIPTMANAGER_H

#include "TailSettings.h"

#include <QString>
#include <QStringList>

class ScriptManager : public QObject
{
    Q_OBJECT
public:
    explicit ScriptManager(TailSettings& s, QObject *parent = nullptr)
        : QObject(parent)
        , settings(s)
    {}
    QString userScriptsDir();

    QStringList getDefinedScripts();

    void reloadScripts();

signals:
    void scriptsUpdated();

private:
    TailSettings& settings;
};

#endif // SCRIPTMANAGER_H
