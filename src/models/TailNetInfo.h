#ifndef TAILNETINFO_H
#define TAILNETINFO_H

#include <QObject>
#include <QString>
#include "JsonHelpers.h"

class TailNetInfo final : public QObject
{
    Q_OBJECT
public:
    QString name{};
    QString magicDnsSuffix{};
    bool magicDnsEnabled = false;

    TailNetInfo() {
    }

    TailNetInfo(const TailNetInfo& o)
        : name(o.name)
        , magicDnsSuffix(o.magicDnsSuffix)
        , magicDnsEnabled(o.magicDnsEnabled)
    { }

    TailNetInfo& operator = (const TailNetInfo& o) {
        name = o.name;
        magicDnsSuffix = o.magicDnsSuffix;
        magicDnsEnabled = o.magicDnsEnabled;

        return *this;
    }

    static TailNetInfo parse(const QJsonObject& obj) {
        TailNetInfo retVal;

        retVal.name = jsonReadString(obj, "Name");
        retVal.magicDnsSuffix = jsonReadString(obj, "MagicDnsSuffix");
        retVal.magicDnsEnabled = jsonReadBool(obj, "MagicDnsEnabled");

        return retVal;
    }
};

#endif // TAILNETINFO_H
