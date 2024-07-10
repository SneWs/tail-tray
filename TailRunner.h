#ifndef TAILRUNNER_H
#define TAILRUNNER_H

#include <QObject>
#include <QString>
#include <QList>
#include <QProcess>

#include "models.h"
#include "TailSettings.h"

class TailRunner : public QObject
{
    Q_OBJECT
public:
    explicit TailRunner(const TailSettings& s, QObject* parent = nullptr);
    virtual ~TailRunner();

    void checkStatus();

    void start(bool usePkExec = false);
    void stop();

    void setUseTailscaleDns(bool use);
    void setAcceptRoutes(bool accept);
    void allowIncomingConnections(bool allow);
    void setOperator(const QString &username);
    void useExitNode(const TailDeviceInfo* exitNode);

    void setAsExitNode(TailDeviceInfo* thisDevice, bool allowLocalNetworkAccess);

    // For this machine to be a exit node
    void advertiseAsExitNode(bool enabled);
    void exitNodeAllowLanAccess(bool enabled);

    // For this machine to use a exit node
    void useExitNode(const QString& exitNodeName);

private:
    const TailSettings& settings;
    QProcess* pProcess;
    enum class Command {
        Connect,
        Disconnect,
        SettingsChange,
        Status
    };

    Command eCommand;

signals:
    void statusUpdated(TailStatus* newStatus);

private:
    void runCommand(QString cmd, QStringList args, bool jsonResult = false, bool usePkExec = false);
    void onProcessCanReadStdOut();

    void parseStatusResponse(const QJsonObject& obj);
};

#endif // TAILRUNNER_H
