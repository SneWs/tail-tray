#ifndef TAILNETINFO_H
#define TAILNETINFO_H

#include <QObject>
#include <QString>
#include <QJsonObject>
#include <QJsonArray>

class TailNetInfo final : public QObject
{
    Q_OBJECT
public:
    TailNetInfo()
        : name("")
        , magicDnsSuffix("")
        , magicDnsEnabled(false)
    {}

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

    QString name;
    QString magicDnsSuffix;
    bool magicDnsEnabled;

    static TailNetInfo parse(const QJsonObject& obj) {
        TailNetInfo retVal;
        if (obj.contains("Name") && !obj["Name"].isNull())
            retVal.name = obj["Name"].toString();

        if (obj.contains("MagicDNSSuffix") && !obj["MagicDNSSuffix"].isNull())
            retVal.magicDnsSuffix = obj["MagicDNSSuffix"].toString();

        if (obj.contains("MagicDNSEnabled") && !obj["MagicDNSEnabled"].isNull())
            retVal.magicDnsEnabled = obj["MagicDNSEnabled"].toBool();

        return retVal;
    }
};

#endif //TAILNETINFO_H
