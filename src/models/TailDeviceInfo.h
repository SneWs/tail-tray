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
    long long userId = 0;
    QString id{};
    QString publicKey{};
    QString hostName{};
    QString dnsName{};
    QString os{};
    QString curAddr{};
    QString relay{};
    QList<QString> tailscaleIPs{};
    QList<QString> allowedIPs{};
    QList<QString> addrs{};
    QList<QString> peerApiUrl{};
    QList<QString> capabilities{};
    QList<QString> capMap{};
    QDateTime keyExpiry{};
    bool online = false;
    bool exitNode = false;
    bool exitNodeOption = false;
    bool active = false;
    bool inNetworkMap = false;
    bool inMagicSock = false;
    bool inEngine = false;

    TailDeviceInfo() = default;

    TailDeviceInfo(const TailDeviceInfo& other)
        : id(other.id)
        , publicKey(other.publicKey)
        , hostName(other.hostName)
        , dnsName(other.dnsName)
        , os(other.os)
        , userId(other.userId)
        , tailscaleIPs(other.tailscaleIPs)
        , allowedIPs(other.allowedIPs)
        , addrs(other.addrs)
        , curAddr(other.curAddr)
        , relay(other.relay)
        , online(other.online)
        , exitNode(other.exitNode)
        , exitNodeOption(other.exitNodeOption)
        , active(other.active)
        , peerApiUrl(other.peerApiUrl)
        , capabilities(other.capabilities)
        , capMap(other.capMap)
        , inNetworkMap(other.inNetworkMap)
        , inMagicSock(other.inMagicSock)
        , inEngine(other.inEngine)
        , keyExpiry(other.keyExpiry)
    { }

    TailDeviceInfo& operator = (const TailDeviceInfo& other) {
        id = other.id;
        publicKey = other.publicKey;
        hostName = other.hostName;
        dnsName = other.dnsName;
        os = other.os;
        userId = other.userId;
        tailscaleIPs = other.tailscaleIPs;
        allowedIPs = other.allowedIPs;
        addrs = other.addrs;
        curAddr = other.curAddr;
        relay = other.relay;
        online = other.online;
        exitNode = other.exitNode;
        exitNodeOption = other.exitNodeOption;
        active = other.active;
        peerApiUrl = other.peerApiUrl;
        capabilities = other.capabilities;
        capMap = other.capMap;
        inNetworkMap = other.inNetworkMap;
        inMagicSock = other.inMagicSock;
        inEngine = other.inEngine;
        keyExpiry = other.keyExpiry;

        return *this;
    }

    [[nodiscard]] QString getShortDnsName() const {
        static QRegularExpression regex("^[^.]+");  // Match everything before the first dot
        QRegularExpressionMatch match = regex.match(dnsName);

        if (match.hasMatch()) {
            return match.captured(0);  // Return the matched hostname part
        }

        return dnsName;
    }

    static TailDeviceInfo parse(const QJsonObject& obj) {
        TailDeviceInfo self{};

        self.id = jsonReadString(obj, "ID");
        self.publicKey = jsonReadString(obj, "PublicKey");
        self.hostName = jsonReadString(obj, "HostName");
        self.dnsName = jsonReadString(obj, "DNSName");
        self.os = jsonReadString(obj, "OS");
        self.userId = jsonReadLong(obj, "UserID");
        self.exitNode = jsonReadBool(obj, "ExitNode");
        self.exitNodeOption = jsonReadBool(obj, "ExitNodeOption");
        self.online = jsonReadBool(obj, "Online");
        self.active = jsonReadBool(obj, "Active");

        if (obj.contains("KeyExpiry") && !obj["KeyExpiry"].isNull()) {
            self.keyExpiry = QDateTime::fromString(obj["KeyExpiry"].toString(), Qt::DateFormat::ISODate);
        }

        if (obj.contains("TailscaleIPs") && !obj["TailscaleIPs"].isNull()) {
            for (const auto& ab : obj["TailscaleIPs"].toArray()) {
                if (ab.isNull())
                    continue;

                self.tailscaleIPs.emplace_back(ab.toString(""));
            }
        }

        return self;
    }
};

#endif //TAILDEVICEINFO_H
