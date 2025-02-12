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
        auto* newStatus = new TailStatus{};
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

        newStatus->magicDnsSuffix = safeReadStr(obj, "MagicDNSSuffix");
        newStatus->clientVersion = safeReadStr(obj, "ClientVersion");

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
        config->privateNodeKey = safeReadStr(obj, "PrivateNodeKey");
        config->oldPrivateNodeKey = safeReadStr(obj, "OldPrivateNodeKey");
        config->networkLockKey = safeReadStr(obj, "NetworkLockKey");
        config->nodeID = safeReadStr(obj, "NodeID");
        config->user = TailUser::parse(obj["UserProfile"].toObject());
        return config;
    }
};

class CurrentTailPrefs final : public QObject {
    Q_OBJECT
public:
    QString controlURL;
    bool routeAll;
    QString exitNodeId;
    QString exitNodeIp;
    QString internalExitNodePrior;
    bool exitNodeAllowLANAccess;
    bool corpDNS;
    bool runSSH;
    bool runWebClient;
    bool wantRunning;
    bool loggedOut;
    bool shieldsUp;
    QList<QString> advertiseTags;
    QString hostname;
    bool notepadURLs;
    QList<QString> advertiseRoutes;
    QList<QString> advertiseServices;
    bool noSNAT;
    bool noStatefulFiltering;
    int netfilterMode;
    QString operatorUser;
    bool autoUpdate_Check;
    bool autoUpdate_Apply;
    bool appConnector_Advertise;
    bool postureChecking;
    QString netfilterKind;
    QList<QString> driveShares;
    bool allowSingleHosts;
    std::unique_ptr<TailPrefsConfig> config;

    static std::unique_ptr<CurrentTailPrefs> parse(const QJsonObject& obj) {
        auto prefs = std::make_unique<CurrentTailPrefs>();

        prefs->controlURL = safeReadStr(obj, "ControlURL");
        prefs->routeAll = safeReadBool(obj, "RouteAll");
        prefs->exitNodeId = safeReadStr(obj, "ExitNodeID");
        prefs->exitNodeIp = safeReadStr(obj, "ExitNodeIP");
        prefs->internalExitNodePrior = safeReadStr(obj, "InternalExitNodePrior");
        prefs->exitNodeAllowLANAccess = safeReadBool(obj, "ExitNodeAllowLANAccess");
        prefs->corpDNS = safeReadBool(obj, "CorpDNS");
        prefs->runSSH = safeReadBool(obj, "RunSSH");
        prefs->runWebClient = safeReadBool(obj, "RunWebClient");
        prefs->wantRunning = safeReadBool(obj, "WantRunning");
        prefs->loggedOut = safeReadBool(obj, "LoggedOut");
        prefs->shieldsUp = safeReadBool(obj, "ShieldsUp");

        // AdvertiseTags
        if (obj.contains("AdvertiseTags") && !obj["AdvertiseTags"].isNull()) {
            for (const auto& tag : obj["AdvertiseTags"].toArray()) {
                prefs->advertiseTags.emplace_back(tag.toString());
            }
        }

        prefs->hostname = safeReadStr(obj, "Hostname");
        prefs->notepadURLs = safeReadBool(obj, "NotepadURLs");

        // AdvertiseRoutes
        if (obj.contains("AdvertiseRoutes") && !obj["AdvertiseRoutes"].isNull()) {
            for (const auto& route : obj["AdvertiseRoutes"].toArray()) {
                prefs->advertiseRoutes.emplace_back(route.toString());
            }
        }

        // AdvertiseServices
        if (obj.contains("AdvertiseServices") && !obj["AdvertiseServices"].isNull()) {
            for (const auto& route : obj["AdvertiseRoutes"].toArray()) {
                prefs->advertiseRoutes.emplace_back(route.toString());
            }
        }

        prefs->noSNAT = safeReadBool(obj, "NoSNAT");
        prefs->noStatefulFiltering = safeReadBool(obj, "NoStatefulFiltering");
        prefs->netfilterMode = safeReadInt(obj, "NetfilterMode");
        prefs->operatorUser = safeReadStr(obj, "OperatorUser");

        // AutoUpdate
        // TBD

        // AppConnector
        // TBD

        prefs->postureChecking = safeReadBool(obj, "PostureChecking");
        prefs->netfilterKind = safeReadStr(obj, "NetfilterKind");
        prefs->allowSingleHosts = safeReadBool(obj, "AllowSingleHosts");

        // DriveShares
        if (obj.contains("DriveShares") && !obj["DriveShares"].isNull()) {
            for (const auto& drive : obj["DriveShares"].toArray()) {
                prefs->driveShares.emplace_back(drive.toString());
            }
        }

        // Config
        prefs->config = TailPrefsConfig::parse(obj["Config"].toObject());
        return prefs;
    }
};

#endif // TAILSTATUS_H
