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
        for (const auto& ab : obj["TailscaleIPs"].toArray())
            self->tailscaleIPs.emplace_back(ab.toString(""));

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
        for (const auto& ab : thisUser["roles"].toArray())
            user->roles.emplace_back(ab.toString(""));

        return user;
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
    QObject qurrentTailNet;
    QList<QString> certDomains;
    QList<QObject> peers;
    TailUser* user;
    QString clientVersion;

    virtual ~TailStatus()
    {
        delete self;
        delete user;
    }

    static TailStatus* parse(const QJsonObject& obj) {
        auto newStatus = new TailStatus{};
        newStatus->version = obj["Version"].toString("");
        newStatus->tun = obj["TUN"].toBool(false);
        newStatus->backendState = obj["backendState"].toString("");
        newStatus->haveNodeKey = obj["HaveNodeKey"].toBool(false);
        newStatus->authUrl = obj["AuthURL"].toString("");
        for (const auto& ab : obj["TailscaleIPs"].toArray())
            newStatus->tailscaleIPs.emplace_back(ab.toString(""));

        if (!obj["Health"].isNull())
        {
            for (const auto& ab : obj["Health"].toArray())
                newStatus->health.emplace_back(ab.toString(""));
        }
        newStatus->magicDnsSuffix = obj["MagicDNSSuffix"].toString("");

        for (const auto& ab : obj["CertDomains"].toArray())
            newStatus->certDomains.emplace_back(ab.toString(""));

        newStatus->clientVersion = obj["ClientVersion"].toString("");

        newStatus->self = TailDeviceInfo::parse(obj["Self"].toObject());
        newStatus->user = TailUser::parse(obj["User"].toObject(), newStatus->self->userId);

        return newStatus;
    }
};

#endif //MODELS_H
