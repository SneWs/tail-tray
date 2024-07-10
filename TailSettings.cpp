//
// Created by marcus on 2024-07-10.
//

#include "TailSettings.h"

#include <QSettings>

TailSettings::TailSettings(QObject* parent)
    : QObject(parent) {
}

bool TailSettings::useTailscaleDns() const {
    return settings.value("useTailscaleDns", true).toBool();
}

void TailSettings::useTailscaleDns(bool use) {
    settings.setValue("useTailscaleDns", use);
}

bool TailSettings::acceptRoutes() const {
    return settings.value("acceptRoutes", true).toBool();
}

void TailSettings::acceptRoutes(bool accept) {
    settings.setValue("acceptRoutes", accept);
}

bool TailSettings::allowIncomingConnections() const {
    return settings.value("allowIncomingConnections", true).toBool();
}

void TailSettings::allowIncomingConnections(bool allow) {
    settings.setValue("allowIncomingConnections", allow);
}

bool TailSettings::advertiseAsExitNode() const {
    return settings.value("advertiseAsExitNode", false).toBool();
}

void TailSettings::advertiseAsExitNode(bool enabled) {
    settings.setValue("advertiseAsExitNode", enabled);
}

bool TailSettings::exitNodeAllowLanAccess() const {
    return settings.value("exitNodeAllowLanAccess", false).toBool();
}

void TailSettings::exitNodeAllowLanAccess(bool enabled) {
    settings.setValue("exitNodeAllowLanAccess", enabled);
}

bool TailSettings::useSubnets() const {
    return settings.value("useSubnets", false).toBool();
}

void TailSettings::useSubnets(bool enabled) {
    settings.setValue("useSubnets", enabled);
}

bool TailSettings::startOnLogin() const {
    return settings.value("startOnLogin", false).toBool();
}

void TailSettings::startOnLogin(bool enabled) {
    settings.setValue("startOnLogin", enabled);
}

QString TailSettings::exitNodeInUse() const {
    return settings.value("exitNodeInUse", "").toString();
}

void TailSettings::exitNodeInUse(const QString& nodeNameOrIp) {
    settings.setValue("exitNodeInUse", nodeNameOrIp);
}

