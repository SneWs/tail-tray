#ifndef TAIL_PREFS_CONFIG_H
#define TAIL_PREFS_CONFIG_H

#include <QObject>
#include <QString>
#include <QJsonObject>
#include <QJsonArray>
#include <QList>

#include "JsonHelpers.h"
#include "TailUser.h"

using namespace JsonHelpers;

class TailPrefsConfig final : QObject {
    Q_OBJECT
public:
    QString privateNodeKey{};
    QString oldPrivateNodeKey{};
    TailUser user{};
    QString networkLockKey{};
    QString nodeID{};

    TailPrefsConfig() {
    }

    TailPrefsConfig(const TailPrefsConfig& other) {
        privateNodeKey = other.privateNodeKey;
        oldPrivateNodeKey = other.oldPrivateNodeKey;
        user = other.user;
        networkLockKey = other.networkLockKey;
        nodeID = other.nodeID;
    }

    TailPrefsConfig& operator = (const TailPrefsConfig& other) {
        privateNodeKey = other.privateNodeKey;
        oldPrivateNodeKey = other.oldPrivateNodeKey;
        user = other.user;
        networkLockKey = other.networkLockKey;
        nodeID = other.nodeID;

        return *this;
    }

    static TailPrefsConfig parse(const QJsonObject& obj) {
        TailPrefsConfig config{};
        config.privateNodeKey = jsonReadString(obj, "PrivateNodeKey");
        config.oldPrivateNodeKey = jsonReadString(obj, "OldPrivateNodeKey");
        config.networkLockKey = jsonReadString(obj, "NetworkLockKey");
        config.nodeID = jsonReadString(obj, "NodeID");
        config.user = TailUser::parse(obj["UserProfile"].toObject());
        return config;
    }
};

#endif // TAIL_PREFS_CONFIG_H
