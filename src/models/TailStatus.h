#ifndef TAILSTATUS_H
#define TAILSTATUS_H

#include <QObject>
#include <QString>
#include <QJsonObject>
#include <QJsonArray>
#include <QList>

#include "TailDeviceInfo.h"
#include "TailNetInfo.h"
#include "TailUser.h"
#include "TailDriveInfo.h"

#include "JsonHelpers.h"

using namespace JsonHelpers;

class TailStatus final : public QObject
{
    Q_OBJECT
public:
    TailDeviceInfo self{};
    TailNetInfo currentTailNet{};
    TailUser user{};
    QString version{};
    QString backendState{};
    QString authUrl{};
    QString magicDnsSuffix{};
    QString clientVersion{};
    QList<QString> tailscaleIPs{};
    QList<QString> health{};
    QList<QString> certDomains{};
    QList<TailDriveInfo> drives{};
    QList<TailDeviceInfo> peers{};
    bool tun{};
    bool haveNodeKey{};
    bool drivesConfigured = true;

    TailStatus() = default;

    TailStatus(const TailStatus& other)
        : version(other.version)
        , tun(other.tun) 
        , backendState(other.backendState)
        , haveNodeKey(other.haveNodeKey)
        , authUrl(other.authUrl)
        , tailscaleIPs(other.tailscaleIPs)
        , self(other.self)
        , health(other.health)
        , magicDnsSuffix(other.magicDnsSuffix)
        , currentTailNet(other.currentTailNet)
        , certDomains(other.certDomains)
        , peers(other.peers)
        , user(other.user)
        , clientVersion(other.clientVersion)
        , drives(other.drives)
        , drivesConfigured(other.drivesConfigured)
    { }

    TailStatus& operator = (const TailStatus& other) {
        version = other.version;
        tun = other.tun;
        backendState = other.backendState;
        haveNodeKey = other.haveNodeKey;
        authUrl = other.authUrl;
        tailscaleIPs = other.tailscaleIPs;
        self = other.self;
        health = other.health;
        magicDnsSuffix = other.magicDnsSuffix;
        currentTailNet = other.currentTailNet;
        certDomains = other.certDomains;
        peers = other.peers;
        user = other.user;
        clientVersion = other.clientVersion;
        drives = other.drives;
        drivesConfigured = other.drivesConfigured;

        return *this;
    }

    static TailStatus parse(const QJsonObject& obj) {
        TailStatus newStatus{};
        newStatus.version = jsonReadString(obj, "Version");
        newStatus.tun = jsonReadBool(obj, "TUN");
        newStatus.backendState = jsonReadString(obj, "BackendState");
        newStatus.haveNodeKey = jsonReadBool(obj, "HaveNodeKey");
        newStatus.authUrl = jsonReadString(obj, "AuthURL");

        // Will be null when not logged in for example
        if (!obj["TailscaleIPs"].isNull()) {
            for (const auto& ab : obj["TailscaleIPs"].toArray()) {
                if (ab.isNull()) {
                    continue;
                }
                newStatus.tailscaleIPs.emplace_back(ab.toString(""));
            }
        }

        if (obj.contains("Health") && !obj["Health"].isNull())
        {
            for (const auto& ab : obj["Health"].toArray()) {
                if (ab.isNull()) {
                    continue;
                }
                newStatus.health.emplace_back(ab.toString(""));
            }
        }

        newStatus.magicDnsSuffix = jsonReadString(obj, "MagicDNSSuffix");
        newStatus.clientVersion = jsonReadString(obj, "ClientVersion");

        if (obj.contains("CurrentTailnet") && !obj["CurrentTailnet"].isNull()) {
            newStatus.currentTailNet = TailNetInfo::parse(obj["CurrentTailnet"].toObject());
        }

        if (obj.contains("CertDomains") && !obj["CertDomains"].isNull()) {
            for (const auto& ab : obj["CertDomains"].toArray()) {
                newStatus.certDomains.emplace_back(ab.toString(""));
            }
        }

        if (obj.contains("Self") && !obj["Self"].isNull())
            newStatus.self = TailDeviceInfo::parse(obj["Self"].toObject());

        if (obj.contains("User") && !obj["User"].isNull()) {
            newStatus.user = TailUser::parse(obj["User"].toObject(), newStatus.self.userId);
        }
        else {
            newStatus.user = TailUser{};
        }

        // Peers
        if (obj.contains("Peer") && !obj["Peer"].isNull()) {
            auto peerObj = obj["Peer"].toObject();
            for (const auto& child : peerObj) {
                if (child.isNull()) {
                    continue;
                }
                newStatus.peers.push_back(TailDeviceInfo::parse(child.toObject()));
            }
        }

        return newStatus;
    }

    QMap<QString, QMap<QString, QList<TailDeviceInfo>>> getMullvadExitNodesByCountry() const {
        QMap<QString, QMap<QString, QList<TailDeviceInfo>>> countryMap;

        for (const auto& peer : peers) {
            if (!peer.isMullvadExitNode()) {
                continue;
            }

            if (!peer.hasLocationInfo()) {
                continue; // Skip peers without location info
            }

            QString country = peer.location.country;
            QString city = peer.location.city.isEmpty() ? "Unknown" : peer.location.city;

            if (!countryMap.contains(country)) {
                countryMap[country] = QMap<QString, QList<TailDeviceInfo>>();
            }

            countryMap[country][city].push_back(peer);
        }

        return countryMap;
    }
};

#endif // TAILSTATUS_H

