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
        retVal.name = obj["Name"].toString();
        retVal.magicDnsSuffix = obj["MagicDNSSuffix"].toString();
        retVal.magicDnsEnabled = obj["MagicDNSEnabled"].toBool();

        return retVal;
    }
};

#endif //TAILNETINFO_H
