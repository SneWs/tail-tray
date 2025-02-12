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
    // ControlURL is the URL of the control server to use.
    QString controlURL;

    // RouteAll specifies whether to accept subnets advertised by
	// other nodes on the Tailscale network. Note that this does not
	// include default routes (0.0.0.0/0 and ::/0), those are
	// controlled by ExitNodeID/IP below.
    bool routeAll;

    // ExitNodeID and ExitNodeIP specify the node that should be used
	// as an exit node for internet traffic. At most one of these
	// should be non-zero.
	//
	// The preferred way to express the chosen node is ExitNodeID, but
	// in some cases it's not possible to use that ID (e.g. in the
	// linux CLI, before tailscaled has a netmap). For those
	// situations, we allow specifying the exit node by IP, and
	// ipnlocal.LocalBackend will translate the IP into an ID when the
	// node is found in the netmap.
	//
	// If the selected exit node doesn't exist (e.g. it's not part of
	// the current tailnet), or it doesn't offer exit node services, a
	// blackhole route will be installed on the local system to
	// prevent any traffic escaping to the local network.
    QString exitNodeId;
    QString exitNodeIp;

    // InternalExitNodePrior is the most recently used ExitNodeID in string form. It is set by
	// the backend on transition from exit node on to off and used by the
	// backend.
	//
	// As an Internal field, it can't be set by LocalAPI clients, rather it is set indirectly
	// when the ExitNodeID value is zero'd and via the set-use-exit-node-enabled endpoint.
    QString internalExitNodePrior;

    // ExitNodeAllowLANAccess indicates whether locally accessible subnets should be
    // routed directly or via the exit node.
    bool exitNodeAllowLANAccess;

    // CorpDNS specifies whether to install the Tailscale network's
    // DNS configuration, if it exists.
    // NOTE: Maps to Use Tailscale DNS settings
    bool corpDNS;

    // RunSSH bool is whether this node should run an SSH
    // server, permitting access to peers according to the
    // policies as configured by the Tailnet's admin(s).
    bool runSSH;

    // RunWebClient bool is whether this node should expose
    // its web client over Tailscale at port 5252,
    // permitting access to peers according to the
    // policies as configured by the Tailnet's admin(s).
    bool runWebClient;

    // WantRunning indicates whether networking should be active on
    // this node.
    bool wantRunning;

    // LoggedOut indicates whether the user intends to be logged out.
    // There are other reasons we may be logged out, including no valid
    // keys.
    // We need to remember this state so that, on next startup, we can
    // generate the "Login" vs "Connect" buttons correctly, without having
    // to contact the server to confirm our nodekey status first.
    bool loggedOut;

    // ShieldsUp indicates whether to block all incoming connections,
    // regardless of the control-provided packet filter. If false, we
    // use the packet filter as provided. If true, we block incoming
    // connections. This overrides tailcfg.Hostinfo's ShieldsUp.
    bool shieldsUp;

    // AdvertiseTags specifies groups that this node wants to join, for
    // purposes of ACL enforcement. These can be referenced from the ACL
    // security policy. Note that advertising a tag doesn't guarantee that
    // the control server will allow you to take on the rights for that
    // tag.
    QList<QString> advertiseTags;

    // Hostname is the hostname to use for identifying the node. If
    // not set, os.Hostname is used.
    QString hostname;

    // NotepadURLs is a debugging setting that opens OAuth URLs in
    // notepad.exe on Windows, rather than loading them in a browser.
    //
    // apenwarr 2020-04-29: Unfortunately this is still needed sometimes.
    // Windows' default browser setting is sometimes screwy and this helps
    // users narrow it down a bit.
    bool notepadURLs;

#if defined(WINDOWS_BUILD)
    // ForceDaemon specifies whether a platform that normally
    // operates in "client mode" (that is, requires an active user
    // logged in with the GUI app running) should keep running after the
    // GUI ends and/or the user logs out.
    //
    // The only current applicable platform is Windows. This
    // forced Windows to go into "server mode" where Tailscale is
    // running even with no users logged in. This might also be
    // used for macOS in the future. This setting has no effect
    // for Linux/etc, which always operate in daemon mode.
    bool forceDaemon;
#endif

    // AdvertiseRoutes specifies CIDR prefixes to advertise into the
    // Tailscale network as reachable through the current
    // node.
    QList<QString> advertiseRoutes;

    // AdvertiseServices specifies the list of services that this
    // node can serve as a destination for. Note that an advertised
    // service must still go through the approval process from the
    // control server.
    QList<QString> advertiseServices;

    // NoSNAT specifies whether to source NAT traffic going to
    // destinations in AdvertiseRoutes. The default is to apply source
    // NAT, which makes the traffic appear to come from the router
    // machine rather than the peer's Tailscale IP.
    //
    // Disabling SNAT requires additional manual configuration in your
    // network to route Tailscale traffic back to the subnet relay
    // machine.
    //
    // Linux-only.
    bool noSNAT;

    // NoStatefulFiltering specifies whether to apply stateful filtering when
    // advertising routes in AdvertiseRoutes. The default is to not apply
    // stateful filtering.
    //
    // To allow inbound connections from advertised routes, both NoSNAT and
    // NoStatefulFiltering must be true.
    //
    // This is an opt.Bool because it was first added after NoSNAT, with a
    // backfill based on the value of that parameter. The backfill has been
    // removed since then, but the field remains an opt.Bool.
    //
    // Linux-only.
    bool noStatefulFiltering;

    // NetfilterMode specifies how much to manage netfilter rules for
    // Tailscale, if at all.
    int netfilterMode;

    // OperatorUser is the local machine user name who is allowed to
    // operate tailscaled without being root or using sudo.
    QString operatorUser;

    // TODO: Check/Parse in JSON
    // ProfileName is the desired name of the profile. If empty, then the user's
    // LoginName is used. It is only used for display purposes in the client UI
    // and CLI.
    QString profileName;

    // AutoUpdate sets the auto-update preferences for the node agent. See
    // AutoUpdatePrefs docs for more details.
    bool autoUpdate_Check;
    bool autoUpdate_Apply;

    // AppConnector sets the app connector preferences for the node agent. See
    // AppConnectorPrefs docs for more details.
    bool appConnector_Advertise;

    // PostureChecking enables the collection of information used for device
    // posture checks.
    bool postureChecking;

    // NetfilterKind specifies what netfilter implementation to use.
    //
    // Linux-only.
    QString netfilterKind;

    // DriveShares are the configured DriveShares, stored in increasing order
    // by name.
    // TODO: Parse from JSON. Data structure, see https://github.com/tailscale/tailscale/blob/main/drive/remote.go#L34
    QList<QString> driveShares;

    // No longer used...
    bool allowSingleHosts;

    std::unique_ptr<TailPrefsConfig> config;

    [[nodiscard]] bool isExitNode() const { return advertiseRoutes.count() > 0; }

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
