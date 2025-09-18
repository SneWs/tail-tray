#ifndef SCRIPTMANAGER_H
#define SCRIPTMANAGER_H

#include "TailSettings.h"

#include <QString>
#include <QStringList>

class ScriptManager {
public:
    explicit ScriptManager(TailSettings &s) : settings(s) {}
    QString userScriptsDir();

    QStringList listScripts();

private:
    TailSettings& settings;
};

#endif // SCRIPTMANAGER_H
