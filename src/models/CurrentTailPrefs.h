#ifndef TAIL_PREFS_H
#define TAIL_PREFS_H

#include <QObject>
#include <QString>
#include <QJsonObject>
#include <QJsonArray>
#include <QList>

#include "JsonHelpers.h"
#include "TailPrefsConfig.h"

using namespace JsonHelpers;

class CurrentTailPrefs final : public QObject {
    Q_OBJECT
public:
    TailPrefsConfig config{};

    // NetfilterMode specifies how much to manage netfilter rules for
    // Tailscale, if at all.
    int netfilterMode = 0;

    // ControlURL is the URL of the control server to use.
    QString controlURL{};

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
    QString exitNodeId{};
    QString exitNodeIp{};

    // InternalExitNodePrior is the most recently used ExitNodeID in string form. It is set by
	// the backend on transition from exit node on to off and used by the
	// backend.
	//
	// As an Internal field, it can't be set by LocalAPI clients, rather it is set indirectly
	// when the ExitNodeID value is zero'd and via the set-use-exit-node-enabled endpoint.
    QString internalExitNodePrior{};

    // AdvertiseTags specifies groups that this node wants to join, for
    // purposes of ACL enforcement. These can be referenced from the ACL
    // security policy. Note that advertising a tag doesn't guarantee that
    // the control server will allow you to take on the rights for that
    // tag.
    QList<QString> advertiseTags{};

    // Hostname is the hostname to use for identifying the node. If
    // not set, os.Hostname is used.
    QString hostname{};

    // AdvertiseRoutes specifies CIDR prefixes to advertise into the
    // Tailscale network as reachable through the current
    // node.
    QList<QString> advertiseRoutes{};

    // AdvertiseServices specifies the list of services that this
    // node can serve as a destination for. Note that an advertised
    // service must still go through the approval process from the
    // control server.
    QList<QString> advertiseServices{};

    // OperatorUser is the local machine user name who is allowed to
    // operate tailscaled without being root or using sudo.
    QString operatorUser{};

    // TODO: Check/Parse in JSON
    // ProfileName is the desired name of the profile. If empty, then the user's
    // LoginName is used. It is only used for display purposes in the client UI
    // and CLI.
    QString profileName{};

    // NetfilterKind specifies what netfilter implementation to use.
    //
    // Linux-only.
    QString netfilterKind{};

    // DriveShares are the configured DriveShares, stored in increasing order
    // by name.
    // TODO: Parse from JSON. Data structure, see https://github.com/tailscale/tailscale/blob/main/drive/remote.go#L34
    QList<QString> driveShares{};

    // RouteAll specifies whether to accept subnets advertised by
    // other nodes on the Tailscale network. Note that this does not
    // include default routes (0.0.0.0/0 and ::/0), those are
    // controlled by ExitNodeID/IP below.
    bool routeAll = false;

    // ExitNodeAllowLANAccess indicates whether locally accessible subnets should be
    // routed directly or via the exit node.
    bool exitNodeAllowLANAccess = false;

    // CorpDNS specifies whether to install the Tailscale network's
    // DNS configuration, if it exists.
    // NOTE: Maps to Use Tailscale DNS settings
    bool corpDNS = false;

    // RunSSH bool is whether this node should run an SSH
    // server, permitting access to peers according to the
    // policies as configured by the Tailnet's admin(s).
    bool runSSH = false;

    // RunWebClient bool is whether this node should expose
    // its web client over Tailscale at port 5252,
    // permitting access to peers according to the
    // policies as configured by the Tailnet's admin(s).
    bool runWebClient = false;

    // WantRunning indicates whether networking should be active on
    // this node.
    bool wantRunning = false;

    // LoggedOut indicates whether the user intends to be logged out.
    // There are other reasons we may be logged out, including no valid
    // keys.
    // We need to remember this state so that, on next startup, we can
    // generate the "Login" vs "Connect" buttons correctly, without having
    // to contact the server to confirm our nodekey status first.
    bool loggedOut = true;

    // ShieldsUp indicates whether to block all incoming connections,
    // regardless of the control-provided packet filter. If false, we
    // use the packet filter as provided. If true, we block incoming
    // connections. This overrides tailcfg.Hostinfo's ShieldsUp.
    bool shieldsUp = false;

    // NotepadURLs is a debugging setting that opens OAuth URLs in
    // notepad.exe on Windows, rather than loading them in a browser.
    //
    // apenwarr 2020-04-29: Unfortunately this is still needed sometimes.
    // Windows' default browser setting is sometimes screwy and this helps
    // users narrow it down a bit.
    bool notepadURLs = false;

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
    bool forceDaemon = false;
#endif

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
    bool noSNAT = false;

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
    bool noStatefulFiltering = false;

    // AutoUpdate sets the auto-update preferences for the node agent. See
    // AutoUpdatePrefs docs for more details.
    bool autoUpdate_Check = false;
    bool autoUpdate_Apply = false;

    // AppConnector sets the app connector preferences for the node agent. See
    // AppConnectorPrefs docs for more details.
    bool appConnector_Advertise = false;

    // PostureChecking enables the collection of information used for device
    // posture checks.
    bool postureChecking = false;

    // No longer used...
    bool allowSingleHosts = false;
    
    CurrentTailPrefs() = default;

    CurrentTailPrefs(const CurrentTailPrefs& other)
        : controlURL(other.controlURL)
        , routeAll(other.routeAll)
        , exitNodeId(other.exitNodeId)
        , exitNodeIp(other.exitNodeIp)
        , internalExitNodePrior(other.internalExitNodePrior)
        , exitNodeAllowLANAccess(other.exitNodeAllowLANAccess)
        , corpDNS(other.corpDNS)
        , runSSH(other.runSSH)
        , runWebClient(other.runWebClient)
        , wantRunning(other.wantRunning)
        , loggedOut(other.loggedOut)
        , shieldsUp(other.shieldsUp)
        , advertiseTags(other.advertiseTags)
        , hostname(other.hostname)
        , notepadURLs(other.notepadURLs)
#if defined(WINDOWS_BUILD)
        , forceDaemon(other.forceDaemon)
#endif
        , advertiseRoutes(other.advertiseRoutes)
        , advertiseServices(other.advertiseServices)
        , noSNAT(other.noSNAT)
        , noStatefulFiltering(other.noStatefulFiltering)
        , netfilterMode(other.netfilterMode)
        , operatorUser(other.operatorUser)
        , profileName(other.profileName)
        , autoUpdate_Check(other.autoUpdate_Check)
        , autoUpdate_Apply(other.autoUpdate_Apply)
        , appConnector_Advertise(other.appConnector_Advertise)
        , postureChecking(other.postureChecking)
        , netfilterKind(other.netfilterKind)
        , driveShares(other.driveShares)
        , allowSingleHosts(other.allowSingleHosts)
        , config(other.config)
    { }

    CurrentTailPrefs& operator = (const CurrentTailPrefs& other)
    {
        controlURL = other.controlURL;
        routeAll = other.routeAll;
        exitNodeId = other.exitNodeId;
        exitNodeIp = other.exitNodeIp;
        internalExitNodePrior = other.internalExitNodePrior;
        exitNodeAllowLANAccess = other.exitNodeAllowLANAccess;
        corpDNS = other.corpDNS;
        runSSH = other.runSSH;
        runWebClient = other.runWebClient;
        wantRunning = other.wantRunning;
        loggedOut = other.loggedOut;
        shieldsUp = other.shieldsUp;
        advertiseTags = other.advertiseTags;
        hostname = other.hostname;
        notepadURLs = other.notepadURLs;
#if defined(WINDOWS_BUILD)
        forceDaemon = other.forceDaemon;
#endif
        advertiseRoutes = other.advertiseRoutes;
        advertiseServices = other.advertiseServices;
        noSNAT = other.noSNAT;
        noStatefulFiltering = other.noStatefulFiltering;
        netfilterMode = other.netfilterMode;
        operatorUser = other.operatorUser;
        profileName = other.profileName;
        autoUpdate_Check = other.autoUpdate_Check;
        autoUpdate_Apply = other.autoUpdate_Apply;
        appConnector_Advertise = other.appConnector_Advertise;
        postureChecking = other.postureChecking;
        netfilterKind = other.netfilterKind;
        driveShares = other.driveShares;
        allowSingleHosts = other.allowSingleHosts;
        config = other.config;

        return *this;
    }

    [[nodiscard]] bool isExitNode() const { return advertiseRoutes.count() > 0; }

    QList<QString> getFilteredAdvertiseRoutes() const {
        QList<QString> filteredRoutes;
        for (const auto& route : advertiseRoutes) {
            // Filter out default routes
            if (route.contains("::/0") || route.contains("0.0.0.0/0"))
                continue;
            filteredRoutes.emplace_back(route);
        }
        return filteredRoutes;
    }

    static CurrentTailPrefs parse(const QJsonObject& obj) {
        CurrentTailPrefs prefs{};

        prefs.controlURL = jsonReadString(obj, "ControlURL");
        prefs.routeAll = jsonReadBool(obj, "RouteAll");
        prefs.exitNodeId = jsonReadString(obj, "ExitNodeID");
        prefs.exitNodeIp = jsonReadString(obj, "ExitNodeIP");
        prefs.internalExitNodePrior = jsonReadString(obj, "InternalExitNodePrior");
        prefs.exitNodeAllowLANAccess = jsonReadBool(obj, "ExitNodeAllowLANAccess");
        prefs.corpDNS = jsonReadBool(obj, "CorpDNS");
        prefs.runSSH = jsonReadBool(obj, "RunSSH");
        prefs.runWebClient = jsonReadBool(obj, "RunWebClient");
        prefs.wantRunning = jsonReadBool(obj, "WantRunning");
        prefs.loggedOut = jsonReadBool(obj, "LoggedOut");
        prefs.shieldsUp = jsonReadBool(obj, "ShieldsUp");

        if (obj.contains("AutoUpdate") && !obj["AutoUpdate"].isNull()) {
            const auto& autoUpdateObj = obj["AutoUpdate"].toObject();
            prefs.autoUpdate_Check = jsonReadBool(autoUpdateObj, "Check");
            prefs.autoUpdate_Apply = jsonReadBool(autoUpdateObj, "Apply");
        }

        // AdvertiseTags
        if (obj.contains("AdvertiseTags") && !obj["AdvertiseTags"].isNull()) {
            for (const auto& tag : obj["AdvertiseTags"].toArray()) {
                prefs.advertiseTags.emplace_back(tag.toString());
            }
        }

        prefs.hostname = jsonReadString(obj, "Hostname");
        prefs.notepadURLs = jsonReadBool(obj, "NotepadURLs");

        // AdvertiseRoutes
        if (obj.contains("AdvertiseRoutes") && !obj["AdvertiseRoutes"].isNull()) {
            for (const auto& route : obj["AdvertiseRoutes"].toArray()) {
                prefs.advertiseRoutes.emplace_back(route.toString());
            }
        }

        // AdvertiseServices
        if (obj.contains("AdvertiseServices") && !obj["AdvertiseServices"].isNull()) {
            for (const auto& route : obj["AdvertiseRoutes"].toArray()) {
                prefs.advertiseRoutes.emplace_back(route.toString());
            }
        }

        prefs.noSNAT = jsonReadBool(obj, "NoSNAT");
        prefs.noStatefulFiltering = jsonReadBool(obj, "NoStatefulFiltering");
        prefs.netfilterMode = jsonReadInt(obj, "NetfilterMode");
        prefs.operatorUser = jsonReadString(obj, "OperatorUser");

        // AutoUpdate
        // TBD

        // AppConnector
        // TBD

        prefs.postureChecking = jsonReadBool(obj, "PostureChecking");
        prefs.netfilterKind = jsonReadString(obj, "NetfilterKind");
        prefs.allowSingleHosts = jsonReadBool(obj, "AllowSingleHosts");

        // DriveShares
        if (obj.contains("DriveShares") && !obj["DriveShares"].isNull()) {
            for (const auto& drive : obj["DriveShares"].toArray()) {
                prefs.driveShares.emplace_back(drive.toString());
            }
        }

        // Config
        prefs.config = TailPrefsConfig::parse(obj["Config"].toObject());
        return prefs;
    }
};

#endif /* TAIL_PREFS_H */
