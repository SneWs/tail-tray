//
// Created by marcus on 2024-07-10.
//

#ifndef TAILSETTINGS_H
#define TAILSETTINGS_H

#include <QObject>
#include <QSettings>

class TailSettings : public QObject {
    Q_OBJECT
public:
    explicit TailSettings(QObject* parent = nullptr);

    void save() { settings.sync(); }

    [[nodiscard]] bool useTailscaleDns() const;
    void useTailscaleDns(bool use);

    [[nodiscard]] bool acceptRoutes() const;
    void acceptRoutes(bool accept);

    [[nodiscard]] bool allowIncomingConnections() const;
    void allowIncomingConnections(bool allow);

    [[nodiscard]] bool advertiseAsExitNode() const;
    void advertiseAsExitNode(bool enabled);

    [[nodiscard]] bool exitNodeAllowLanAccess() const;
    void exitNodeAllowLanAccess(bool enabled);

    [[nodiscard]] bool startOnLogin() const;
    void startOnLogin(bool enabled);

    [[nodiscard]] QString tailDriveMountPath() const;
    void tailDriveMountPath(const QString& path);

    [[nodiscard]] bool tailDriveEnabled() const;
    void tailDriveEnabled(bool enabled);

    [[nodiscard]] QString tailFilesDefaultSavePath() const;
    void tailFilesDefaultSavePath(const QString &path);

private:
    QSettings settings;
};

#endif //TAILSETTINGS_H
