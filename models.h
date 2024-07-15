//
// Created by marcus on 2024-07-05.
//

#ifndef MODELS_H
#define MODELS_H

#include <memory>

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
    Connected
};

class TailAccountInfo : public QObject
{
    Q_OBJECT
public:
    QString id;
    QString tailnet;
    QString account;

    TailAccountInfo()
        : QObject(nullptr)
        , id("")
        , tailnet("")
        , account("")
    { }

    TailAccountInfo(const TailAccountInfo& o)
        : QObject(nullptr)
        , id(o.id)
        , tailnet(o.tailnet)
        , account(o.account)
    { }

    TailAccountInfo& operator = (const TailAccountInfo& o) {
        id = o.id;
        tailnet = o.tailnet;
        account = o.account;

        return *this;
    }

    static TailAccountInfo parse(const QString& rawLineData) {
        // NOTE: This is a raw line from the command output
        // The format is:
        // ID    Tailnet             Account
        static const QRegularExpression regex(R"((\w{4})\s+(\S+)\s+(\S+))");
        TailAccountInfo retVal;
        QRegularExpressionMatch match = regex.match(rawLineData);
        if (match.hasMatch()) {
            retVal.id = match.captured(1);
            retVal.tailnet = match.captured(2);
            retVal.account = match.captured(3);
        }

        return retVal;
    }

    // This handles fragmented lines as well from the command line output
    // so even if a line starts with the ID and then break it will make sure to parse it correctly
    static QList<TailAccountInfo> parseAllFound(const QString& rawData) {
        QList<TailAccountInfo> retVal;

        // Split input into individual lines
        const QStringList lines = rawData.split('\n', Qt::SkipEmptyParts);

        QStringList entries;
        QString currentEntry;

        // Reconstruct broken lines into complete entries
        // And we skip the first line since it's the header line
        for (int i = 1; i < lines.length(); i++) {
            const QString& line = lines[i].trimmed();
            if (isIdLine(line)) {
                if (!currentEntry.isEmpty()) {
                    entries.append(currentEntry);
                    currentEntry.clear();
                }
                currentEntry = line;
            }
            else {
                currentEntry += "\t" + line.trimmed();
            }
        }

        if (!currentEntry.isEmpty()) {
            entries.append(currentEntry);
        }

        // Iterate over each reconstructed entry and apply the regular expression
        for (const QString& entry : entries) {
            qDebug() << "Parsing entry: " << entry;
            retVal.emplace_back(TailAccountInfo::parse(entry));
        }

        return retVal;
    }

    static bool isIdLine(const QString& line) {
        // Check if the line starts with a 4-character word
        static const QRegularExpression idRe(R"(^\w{4}\s+)");
        return idRe.match(line).hasMatch();
    }
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

    static std::unique_ptr<TailDeviceInfo> parse(const QJsonObject& obj)
    {
        auto self = std::make_unique<TailDeviceInfo>();

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

    static std::unique_ptr<TailUser> parse(const QJsonObject& obj, long long userId) {
        auto user = std::make_unique<TailUser>();

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
    std::unique_ptr<TailDeviceInfo> self;
    QList<QString> health;
    QString magicDnsSuffix;
    TailNetInfo currentTailNet;
    QList<QString> certDomains;
    QList<std::shared_ptr<TailDeviceInfo>> peers;
    std::unique_ptr<TailUser> user;
    QString clientVersion;

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
            newStatus->user = std::make_unique<TailUser>();
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
