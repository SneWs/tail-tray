#ifndef TAILSTATUS_H
#define TAILSTATUS_H

#include <QObject>
#include <QString>
#include <QJsonObject>
#include <QJsonArray>
#include <QList>

#include <memory>
#include <vector>

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
    QString version{};
    bool tun{};
    QString backendState{};
    bool haveNodeKey{};
    QString authUrl{};
    QList<QString> tailscaleIPs{};
    TailDeviceInfo self{};
    QList<QString> health{};
    QString magicDnsSuffix{};
    TailNetInfo currentTailNet{};
    QList<QString> certDomains{};
    std::vector<TailDeviceInfo> peers{};
    std::unique_ptr<TailUser> user{};
    QString clientVersion{};

    // Drives that has been shared...
    QList<TailDriveInfo> drives{};
    bool drivesConfigured = true;

    static TailStatus* parse(const QJsonObject& obj) {
        auto* newStatus = new TailStatus{};
        newStatus->version = jsonReadString(obj, "Version");
        newStatus->tun = jsonReadBool(obj, "TUN");
        newStatus->backendState = jsonReadString(obj, "BackendState");
        newStatus->haveNodeKey = jsonReadBool(obj, "HaveNodeKey");
        newStatus->authUrl = jsonReadString(obj, "AuthURL");

        // Will be null when not logged in for example
        if (!obj["TailscaleIPs"].isNull()) {
            for (const auto& ab : obj["TailscaleIPs"].toArray()) {
                if (ab.isNull())
                    continue;
                newStatus->tailscaleIPs.emplace_back(ab.toString(""));
            }
        }

        if (obj.contains("Health") && !obj["Health"].isNull())
        {
            for (const auto& ab : obj["Health"].toArray()) {
                if (ab.isNull())
                    continue;
                newStatus->health.emplace_back(ab.toString(""));
            }
        }

        newStatus->magicDnsSuffix = jsonReadString(obj, "MagicDNSSuffix");
        newStatus->clientVersion = jsonReadString(obj, "ClientVersion");

        if (obj.contains("CurrentTailnet") && !obj["CurrentTailnet"].isNull()) {
            newStatus->currentTailNet = TailNetInfo::parse(obj["CurrentTailnet"].toObject());
        }

        if (obj.contains("CertDomains") && !obj["CertDomains"].isNull()) {
            for (const auto& ab : obj["CertDomains"].toArray()) {
                newStatus->certDomains.emplace_back(ab.toString(""));
            }
        }

        if (obj.contains("Self") && !obj["Self"].isNull())
            newStatus->self = TailDeviceInfo::parse(obj["Self"].toObject());

        if (obj.contains("User") && !obj["User"].isNull()) {
            newStatus->user = TailUser::parse(obj["User"].toObject(), newStatus->self.userId);
        }
        else {
            newStatus->user = std::make_unique<TailUser>();
        }

        // Peers
        if (obj.contains("Peer") && !obj["Peer"].isNull()) {
            auto peerObj = obj["Peer"].toObject();
            for (const auto& child : peerObj) {
                if (child.isNull())
                    continue;
                newStatus->peers.push_back(TailDeviceInfo::parse(child.toObject()));
            }
        }

        return newStatus;
    }
};

class TailPrefsConfig final : QObject {
    Q_OBJECT
public:
    QString privateNodeKey;
    QString oldPrivateNodeKey;
    std::unique_ptr<TailUser> user;
    QString networkLockKey;
    QString nodeID;

    static std::unique_ptr<TailPrefsConfig> parse(const QJsonObject& obj) {
        auto config = std::make_unique<TailPrefsConfig>();
        config->privateNodeKey = jsonReadString(obj, "PrivateNodeKey");
        config->oldPrivateNodeKey = jsonReadString(obj, "OldPrivateNodeKey");
        config->networkLockKey = jsonReadString(obj, "NetworkLockKey");
        config->nodeID = jsonReadString(obj, "NodeID");
        config->user = TailUser::parse(obj["UserProfile"].toObject());
        return config;
    }
};


#endif // TAILSTATUS_H
