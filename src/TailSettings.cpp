#include "TailSettings.h"

#include <QDir>
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

bool TailSettings::startOnLogin() const {
    return settings.value("startOnLogin", false).toBool();
}

void TailSettings::startOnLogin(bool enabled) {
    settings.setValue("startOnLogin", enabled);
}

bool TailSettings::autoUpdateTailscale() const {
    return settings.value("autoUpdateTailscale", true).toBool();
}
void TailSettings::autoUpdateTailscale(bool enabled) {
    settings.setValue("autoUpdateTailscale", enabled);
}

QString TailSettings::tailDriveMountPath() const {
    auto homePath = qEnvironmentVariable("HOME");
    return settings.value("tailDriveMountPath", homePath.append("/Tailscale")).toString();
}

void TailSettings::tailDriveMountPath(const QString& path) {
    settings.setValue("tailDriveMountPath", path);
}

bool TailSettings::tailDriveEnabled() const {
    return settings.value("tailDriveEnabled", false).toBool();
}

void TailSettings::tailDriveEnabled(bool enabled) {
    settings.setValue("tailDriveEnabled", enabled);
}

QString TailSettings::tailFilesDefaultSavePath() const {
    auto defaultSaveDir = QDir::home();
    if (!defaultSaveDir.cd("Downloads"))
        defaultSaveDir = QDir::home();

    auto savePath = settings.value("tailFilesDefaultSavePath", defaultSaveDir.absolutePath()).toString();
    if (!QDir(savePath).exists())
        return QDir::homePath();

    return savePath;
}

void TailSettings::tailFilesDefaultSavePath(const QString& path) {
    settings.setValue("tailFilesDefaultSavePath", path);
}
