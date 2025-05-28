#ifndef TAILDEVICEINFO_H
#define TAILDEVICEINFO_H

#include <QObject>
#include <QString>
#include <QDateTime>
#include <QList>
#include <QJsonObject>
#include <QJsonArray>

#include "JsonHelpers.h"

using namespace JsonHelpers;

class TailDeviceLocationInfo final : public QObject
{
    Q_OBJECT
public:
    QString country{};
    QString countryCode{};
    QString city{};
    QString cityCode{};
    float latitude = 0.0f;
    float longitude = 0.0f;
    int priority = 100;

    TailDeviceLocationInfo() = default;

    TailDeviceLocationInfo(const TailDeviceLocationInfo& other)
        : country(other.country)
        , countryCode(other.countryCode)
        , city(other.city)
        , cityCode(other.cityCode)
        , latitude(other.latitude)
        , longitude(other.longitude)
        , priority(other.priority)
    { }

    TailDeviceLocationInfo& operator = (const TailDeviceLocationInfo& other) {
        country = other.country;
        countryCode = other.countryCode;
        city = other.city;
        cityCode = other.cityCode;
        latitude = other.latitude;
        longitude = other.longitude;
        priority = other.priority;

        return *this;
    }
};

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
    QList<QString> tags{};
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
    TailDeviceLocationInfo location{};

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
        , tags(other.tags)
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
        , location(other.location)
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
        tags = other.tags;
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
        location = other.location;

        return *this;
    }

    [[nodiscard]] bool isMullvadExitNode() const {
        return exitNodeOption && tags.contains("tag:mullvad-exit-node", Qt::CaseInsensitive);
    }

    [[nodiscard]] bool hasLocationInfo() const {
        return !location.country.isEmpty();
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

        if (obj.contains("Tags") && !obj["Tags"].isNull()) {
            for (const auto& ab : obj["Tags"].toArray()) {
                if (ab.isNull()) {
                    continue;
                }

                self.tags.emplace_back(ab.toString());
            }
        }
        else {
            //qDebug() << "No tags found for device";
        }

        if (obj.contains("Location") && !obj["Location"].isNull()) {
            const auto& locationObj = obj["Location"].toObject();
            self.location.country = jsonReadString(locationObj, "Country");
            self.location.countryCode = jsonReadString(locationObj, "CountryCode");
            self.location.city = jsonReadString(locationObj, "City");
            self.location.cityCode = jsonReadString(locationObj, "CityCode");
            self.location.latitude = jsonReadFloat(locationObj, "Latitude");
            self.location.longitude = jsonReadFloat(locationObj, "Longitude");
            self.location.priority = jsonReadInt(locationObj, "Priority");
        }
        else {
            //qDebug() << "No location data found for device";
        }

        return self;
    }
};

#endif //TAILDEVICEINFO_H
