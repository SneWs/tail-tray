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

    [[nodiscard]] bool useSubnets() const;
    void useSubnets(bool enabled);

    [[nodiscard]] bool startOnLogin() const;
    void startOnLogin(bool enabled);

private:
    QSettings settings;
};

#endif //TAILSETTINGS_H
