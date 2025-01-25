#ifndef TAILDEVICEINFO_H
#define TAILDEVICEINFO_H

#include <QObject>
#include <QString>
#include <QDateTime>
#include <QList>
#include <QJsonObject>
#include <QJsonArray>

#include <memory>

#include "JsonHelpers.h"

using namespace JsonHelpers;

class TailDeviceInfo final : public QObject
{
    Q_OBJECT
public:
    QString id;
    QString publicKey;
    QString hostName;
    QString dnsName;
    QString os;
    long long userId;
    QList<QString> tailscaleIPs;
    QList<QString> allowedIPs;
    QList<QString> addrs;
    QString curAddr;
    QString relay;
    bool online;
    bool exitNode;
    bool exitNodeOption;
    bool active;
    QList<QString> peerApiUrl;
    QList<QString> capabilities;
    QList<QString> capMap;
    bool inNetworkMap;
    bool inMagicSock;
    bool inEngine;
    QDateTime keyExpiry;

    [[nodiscard]] QString getShortDnsName() const {
        static QRegularExpression regex("^[^.]+");  // Match everything before the first dot
        QRegularExpressionMatch match = regex.match(dnsName);

        if (match.hasMatch()) {
            return match.captured(0);  // Return the matched hostname part
        }

        return dnsName;
    }

    static std::unique_ptr<TailDeviceInfo> parse(const QJsonObject& obj)
    {
        auto self = std::make_unique<TailDeviceInfo>();

        self->id = safeReadStr(obj, "ID");
        self->publicKey = safeReadStr(obj, "PublicKey");
        self->hostName = safeReadStr(obj, "HostName");
        self->dnsName = safeReadStr(obj, "DNSName");
        self->os = safeReadStr(obj, "OS");
        self->userId = safeReadLong(obj, "UserID");
        self->exitNode = safeReadBool(obj, "ExitNode");
        self->exitNodeOption = safeReadBool(obj, "ExitNodeOption");
        self->online = safeReadBool(obj, "Online");
        self->active = safeReadBool(obj, "Active");

        if (obj.contains("KeyExpiry") && !obj["KeyExpiry"].isNull()) {
            self->keyExpiry = QDateTime::fromString(obj["KeyExpiry"].toString(), Qt::DateFormat::ISODate);
        }

        if (obj.contains("TailscaleIPs") && !obj["TailscaleIPs"].isNull()) {
            for (const auto& ab : obj["TailscaleIPs"].toArray()) {
                if (ab.isNull())
                    continue;

                self->tailscaleIPs.emplace_back(ab.toString(""));
            }
        }

        return self;
    }
};

#endif //TAILDEVICEINFO_H
