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
        newStatus->version = obj["Version"].toString("");
        newStatus->tun = obj["TUN"].toBool(false);
        newStatus->backendState = obj["BackendState"].toString("");
        newStatus->haveNodeKey = obj["HaveNodeKey"].toBool(false);
        newStatus->authUrl = obj["AuthURL"].toString("");

        // Will be null when not logged in for example
        if (!obj["TailscaleIPs"].isNull()) {
            for (const auto& ab : obj["TailscaleIPs"].toArray()) {
                newStatus->tailscaleIPs.emplace_back(ab.toString(""));
            }
        }

        if (!obj["Health"].isNull())
        {
            for (const auto& ab : obj["Health"].toArray()) {
                newStatus->health.emplace_back(ab.toString(""));
            }
        }

        newStatus->magicDnsSuffix = obj["MagicDNSSuffix"].toString("");

        if (obj.contains("CurrentTailnet") && !obj["CurrentTailnet"].isNull()) {
            newStatus->currentTailNet = TailNetInfo::parse(obj["CurrentTailnet"].toObject());
        }

        if (!obj["CertDomains"].isNull()) {
            for (const auto& ab : obj["CertDomains"].toArray()) {
                newStatus->certDomains.emplace_back(ab.toString(""));
            }
        }

        if (!obj["ClientVersion"].isNull()) {
            newStatus->clientVersion = obj["ClientVersion"].toString("");
        }

        newStatus->self = TailDeviceInfo::parse(obj["Self"].toObject());
        if (obj["User"].isNull()) {
            newStatus->user = std::make_unique<TailUser>();
        }
        else {
            newStatus->user = TailUser::parse(obj["User"].toObject(), newStatus->self->userId);
        }

        // Peers
        if (!obj["Peer"].isNull()) {
            auto peerObj = obj["Peer"].toObject();
            for (const auto& child : peerObj) {
                newStatus->peers.emplace_back(TailDeviceInfo::parse(child.toObject()));
            }
        }

        return newStatus;
    }
};

#endif //TAILSTATUS_H
