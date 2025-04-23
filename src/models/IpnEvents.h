#ifndef IPN_EVENTS_H
#define IPN_EVENTS_H

#include <QObject>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QString>

#include "JsonHelpers.h"

using namespace JsonHelpers;

struct IpnHealthWarningsNetworkStatusModel {
    QString BrokenSince;
    QList<QString> DependsOn;
    bool ImpactsConnectivity;
    QString Severity;
    QString Text;
    QString Title;
    QString WarnableCode;

    static IpnHealthWarningsNetworkStatusModel parse(const QJsonObject& json) {
        IpnHealthWarningsNetworkStatusModel model;
        model.BrokenSince = JsonHelpers::jsonReadString(json, "BrokenSince");
        
        // Parse DependsOn array
        model.DependsOn = {};
        if (json.contains("DependsOn") && !json["DependsOn"].isNull()) {
            QJsonArray dependsOnArray = json["DependsOn"].toArray();
            for (const QJsonValue& value : dependsOnArray) {
                if (value.isNull())
                    continue;
                model.DependsOn.append(value.toString());
            }
        }

        model.ImpactsConnectivity = JsonHelpers::jsonReadBool(json, "ImpactsConnectivity");
        model.Severity = JsonHelpers::jsonReadString(json, "Severity");
        model.Text = JsonHelpers::jsonReadString(json, "Text");
        model.Title = JsonHelpers::jsonReadString(json, "Title");
        model.WarnableCode = JsonHelpers::jsonReadString(json, "WarnableCode");
        
        return model;
    }
};

struct IpnHealthWarningsModel {
    IpnHealthWarningsNetworkStatusModel networkStatus;

    static IpnHealthWarningsModel parse(const QJsonObject& json) {
        IpnHealthWarningsModel model;
        if (json.contains("network-status") && !json["network-status"].isNull()) {
            model.networkStatus = IpnHealthWarningsNetworkStatusModel::parse(json["network-status"].toObject());
        }
        else {
            model.networkStatus = IpnHealthWarningsNetworkStatusModel{};
        }
        return model;
    }
};

struct IpnHealthModel {
    IpnHealthWarningsModel Warnings;

    static IpnHealthModel parse(const QJsonObject& json) {
        IpnHealthModel model;
        if (json.contains("Warnings") && !json["Warnings"].isNull()) {
            model.Warnings = IpnHealthWarningsModel::parse(json["Warnings"].toObject());
        }
        else {
            model.Warnings = IpnHealthWarningsModel{};
        }
        return model;
    }
};

class IpnEventData final : public QObject {
    Q_OBJECT
public:
    QString BrowseToURL;
    QString DriveShares;
    QString Engine;
    QString ErrMessage;
    IpnHealthModel Health;
    QString LoginFinished;
    QString NetMap;
    QString Prefs;
    QString State;
    QString Version;

    IpnEventData()
        : BrowseToURL()
        , DriveShares()
        , Engine()
        , ErrMessage()
        , Health()
        , LoginFinished()
        , NetMap()
        , Prefs()
        , State()
        , Version()
    { }

    IpnEventData(const IpnEventData& other)
        : BrowseToURL(other.BrowseToURL)
        , DriveShares(other.DriveShares)
        , Engine(other.Engine)
        , ErrMessage(other.ErrMessage)
        , Health(other.Health)
        , LoginFinished(other.LoginFinished)
        , NetMap(other.NetMap)
        , Prefs(other.Prefs)
        , State(other.State)
        , Version(other.Version)
    { }

    IpnEventData& operator = (const IpnEventData& other) {
        BrowseToURL = other.BrowseToURL;
        DriveShares = other.DriveShares;
        Engine = other.Engine;
        ErrMessage = other.ErrMessage;
        Health = other.Health;
        LoginFinished = other.LoginFinished;
        NetMap = other.NetMap;
        Prefs = other.Prefs;
        State = other.State;
        Version = other.Version;

        return *this;
    }

    // Deserialize from QJsonObject
    static IpnEventData parse(const QJsonObject& json) {
        IpnEventData model{};
        model.BrowseToURL = JsonHelpers::jsonReadString(json, "BrowseToURL");
        model.DriveShares = JsonHelpers::jsonReadString(json, "DriveShares");
        model.Engine = JsonHelpers::jsonReadString(json, "Engine");
        model.ErrMessage = JsonHelpers::jsonReadString(json, "ErrMessage");
        if (json.contains("Health") && !json["Health"].isNull()) {
            model.Health = IpnHealthModel::parse(json["Health"].toObject());
        }
        else {
            model.Health = IpnHealthModel{};
        }
        model.LoginFinished = JsonHelpers::jsonReadString(json, "LoginFinished");
        // model.NetMap = json["NetMap"].toString();
        // model.Prefs = json["Prefs"].toString();
        model.State = json["State"].toString();
        model.Version = JsonHelpers::jsonReadString(json, "Version");

        return model;
    }
};

#endif /* IPN_EVENTS_H */
