#ifndef TAILSTATUS_H
#define TAILSTATUS_H

#include <QObject>
#include <QString>
#include <QJsonObject>
#include <QJsonArray>
#include <QList>

#include <memory>

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
    QString version;
    bool tun{};
    QString backendState;
    bool haveNodeKey{};
    QString authUrl;
    QList<QString> tailscaleIPs;
    std::unique_ptr<TailDeviceInfo> self;
    QList<QString> health;
    QString magicDnsSuffix;
    TailNetInfo currentTailNet;
    QList<QString> certDomains;
    QList<std::shared_ptr<TailDeviceInfo>> peers;
    std::unique_ptr<TailUser> user;
    QString clientVersion;

    // Drives that has been shared...
    QList<TailDriveInfo> drives = {};
    bool drivesConfigured = true;

    static TailStatus* parse(const QJsonObject& obj) {
        const auto newStatus = new TailStatus{};
        newStatus->version = safeReadStr(obj, "Version");
        newStatus->tun = safeReadBool(obj, "TUN");
        newStatus->backendState = safeReadStr(obj, "BackendState");
        newStatus->haveNodeKey = safeReadBool(obj, "HaveNodeKey");
        newStatus->authUrl = safeReadStr(obj, "AuthURL");

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

        if (obj.contains("MagicDNSSuffix"))
            newStatus->magicDnsSuffix = safeReadStr(obj, "MagicDNSSuffix");

        if (obj.contains("CurrentTailnet") && !obj["CurrentTailnet"].isNull()) {
            newStatus->currentTailNet = TailNetInfo::parse(obj["CurrentTailnet"].toObject());
        }

        if (obj.contains("CertDomains") && !obj["CertDomains"].isNull()) {
            for (const auto& ab : obj["CertDomains"].toArray()) {
                newStatus->certDomains.emplace_back(ab.toString(""));
            }
        }

        if (obj.contains("ClientVersion") && !obj["ClientVersion"].isNull()) {
            newStatus->clientVersion = obj["ClientVersion"].toString("");
        }

        if (obj.contains("Self") && !obj["Self"].isNull())
            newStatus->self = TailDeviceInfo::parse(obj["Self"].toObject());

        if (obj.contains("User") && !obj["User"].isNull()) {
            newStatus->user = TailUser::parse(obj["User"].toObject(), newStatus->self->userId);
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
                newStatus->peers.emplace_back(TailDeviceInfo::parse(child.toObject()));
            }
        }

        return newStatus;
    }
};

#endif //TAILSTATUS_H
