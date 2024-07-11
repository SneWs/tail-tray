//
// Created by marcus on 2024-07-05.
//

#ifndef MODELS_H
#define MODELS_H

#include <QObject>
#include <QString>
#include <QList>
#include <QDateTime>
#include <QJsonObject>
#include <QJsonArray>

enum class TailState {
    NoAccount,
    NotLoggedIn,
    LoggedIn,
    NotConnected,
    Connected,
    ConnectedWithExitNode
};

class TailDeviceInfo : public QObject
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

    static TailDeviceInfo* parse(const QJsonObject& obj)
    {
        auto self = new TailDeviceInfo{};

        self->id = obj["ID"].toString("");
        self->publicKey = obj["PublicKey"].toString("");
        self->hostName = obj["HostName"].toString("");
        self->dnsName = obj["DNSName"].toString("");
        self->os = obj["OS"].toString("");
        self->userId = obj["UserID"].toInteger();
        self->exitNode = obj["ExitNode"].toBool();
        self->exitNodeOption = obj["ExitNodeOption"].toBool();
        self->online = obj["Online"].toBool();
        self->active = obj["Active"].toBool();

        if (!obj["KeyExpiry"].isNull()) {
            self->keyExpiry = QDateTime::fromString(obj["KeyExpiry"].toString(), Qt::DateFormat::ISODate);
        }

        if (!obj["TailscaleIPs"].isNull()) {
            for (const auto& ab : obj["TailscaleIPs"].toArray()) {
                self->tailscaleIPs.emplace_back(ab.toString(""));
            }
        }

        return self;
    }
};

class TailUser : public QObject
{
    Q_OBJECT
public:
    long long id;
    QString loginName;
    QString displayName;
    QString profilePicUrl;
    QList<QString> roles;

    static TailUser* parse(const QJsonObject& obj, long long userId) {
        auto user = new TailUser{};

        // Get the user object based on the user id (user id comes from the self part in the same doc)
        auto thisUser = obj[QString::number(userId)].toObject();

        user->id = thisUser["ID"].toInteger(0L);
        user->loginName = thisUser["LoginName"].toString("");
        user->displayName = thisUser["DisplayName"].toString("");
        user->profilePicUrl = thisUser["ProfilePicURL"].toString("");

        for (const auto& ab : thisUser["roles"].toArray()) {
            user->roles.emplace_back(ab.toString(""));
        }

        return user;
    }
};

class TailNetInfo : public QObject
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

class TailStatus : public QObject
{
    Q_OBJECT
public:
    QString version;
    bool tun;
    QString backendState;
    bool haveNodeKey;
    QString authUrl;
    QList<QString> tailscaleIPs;
    TailDeviceInfo* self;
    QList<QString> health;
    QString magicDnsSuffix;
    TailNetInfo currentTailNet;
    QList<QString> certDomains;
    QList<TailDeviceInfo*> peers;
    TailUser* user;
    QString clientVersion;

    virtual ~TailStatus()
    {
        for (auto* peer : peers)
            delete peer;
        peers.clear();

        delete self;
        delete user;
    }

    static TailStatus* parse(const QJsonObject& obj) {
        auto newStatus = new TailStatus{};
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
            newStatus->user = new TailUser{};
        }
        else {
            newStatus->user = TailUser::parse(obj["User"].toObject(), newStatus->self->userId);
        }

        // Peers
        if (!obj["Peer"].isNull()) {
            auto pobj = obj["Peer"].toObject();
            for (const auto& child : pobj) {
                newStatus->peers.emplace_back(TailDeviceInfo::parse(child.toObject()));
            }
        }

        return newStatus;
    }
};

#endif //MODELS_H
