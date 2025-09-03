#ifndef SCRIPTMANAGER_H
#define SCRIPTMANAGER_H

#include <QString>
#include <QStringList>

class ScriptManager {
public:
    static QString userScriptsDir();

    static void ensureScriptsDirExist();

    static QStringList listScripts();
};

#endif // SCRIPTMANAGER_H
