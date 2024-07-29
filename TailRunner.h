#ifndef TAILRUNNER_H
#define TAILRUNNER_H

#include <memory>

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

    void checkStatus();
    void getAccounts();
    void switchAccount(const QString& accountId);

    void login();
    void logout();

    void start(bool usePkExec = false);
    void stop();

    void listDrives();
    void addDrive(const TailDriveInfo& drive);
    void renameDrive(const TailDriveInfo& drive, const QString& newName);
    void removeDrive(const TailDriveInfo& drive);

    void sendFile(const QString& targetDevice, const QString& localFilePath, void* userData = nullptr);

private:
    const TailSettings& settings;
    std::unique_ptr<QProcess> pProcess;
    enum class Command {
        ListAccounts,
        SwitchAccount,
        Login,
        Logout,
        Connect,
        Disconnect,
        SettingsChange,
        Status,
        Drive,
        DriveAdd,
        DriveRename,
        DriveRemove,
        SendFile,
    };

    Command eCommand;

signals:
    void accountsListed(const QList<TailAccountInfo>& accounts);
    void statusUpdated(TailStatus* newStatus);
    void loginFlowCompleted();
    void driveListed(const QList<TailDriveInfo>& drives, bool error, const QString& errorMsg);
    void fileSent(bool success, const QString& errorMsg, void* userData);

private:
    void runCommand(const QString& cmd, QStringList args, bool jsonResult = false, bool usePkExec = false, void* userData = nullptr);
    void onProcessCanReadStdOut();

    void parseStatusResponse(const QJsonObject& obj);
};

#endif // TAILRUNNER_H
