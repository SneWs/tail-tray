#ifndef SCRIPTMANAGER_H
#define SCRIPTMANAGER_H

#include "TailSettings.h"

#include <QString>
#include <QStringList>

class ScriptManager {
public:
    explicit ScriptManager(TailSettings &s)
        : settings(s)
    {}

    [[nodiscard]] QString userScriptsDir() const;
    [[nodiscard]] QStringList getDefinedScripts() const;

private:
    TailSettings& settings;
};

#endif // SCRIPTMANAGER_H
