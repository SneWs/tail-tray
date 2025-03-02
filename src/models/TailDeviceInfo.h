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

        self->id = jsonReadString(obj, "ID");
        self->publicKey = jsonReadString(obj, "PublicKey");
        self->hostName = jsonReadString(obj, "HostName");
        self->dnsName = jsonReadString(obj, "DNSName");
        self->os = jsonReadString(obj, "OS");
        self->userId = jsonReadLong(obj, "UserID");
        self->exitNode = jsonReadBool(obj, "ExitNode");
        self->exitNodeOption = jsonReadBool(obj, "ExitNodeOption");
        self->online = jsonReadBool(obj, "Online");
        self->active = jsonReadBool(obj, "Active");

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
