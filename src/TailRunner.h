#ifndef TAILRUNNER_H
#define TAILRUNNER_H

#include <memory>

#include <QObject>
#include <QString>
#include <QList>
#include <QProcess>

#include "models/Models.h"
#include "TailSettings.h"

enum class Command {
    SetOperator,
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
    SendFile
};

/// Simple way of abstracting away and be able to lazily delete and reference a process and it's states
class ProcessWrapper : public QObject
{
    Q_OBJECT
public:
    explicit ProcessWrapper(Command cmd, QObject* parent = nullptr);

    /// Start the process with the given command and arguments
    void start(const QString& cmd, QStringList args, bool jsonResult, bool usePkExec, void* userData);

    [[nodiscard]] QProcess* process() const { return proc.get();}
    [[nodiscard]] Command command() const { return eCommand; }
    [[nodiscard]] void* userData() const { return pUserData; }
    [[nodiscard]] bool isCompleted() const { return completed; }

signals:
    void processCanReadStdOut(ProcessWrapper* process);
    void processCanReadStandardError(ProcessWrapper* process);
    void processFinished(ProcessWrapper* process, int exitCode, QProcess::ExitStatus exitStatus);

private slots:
    void onProcessCanReadStdOut();
    void onProcessCanReadStandardError();
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);

private:
    std::unique_ptr<QProcess> proc;
    void* pUserData;
    Command eCommand;
    bool completed;
};

class TailRunner : public QObject
{
    Q_OBJECT
public:
    explicit TailRunner(const TailSettings& s, QObject* parent = nullptr);

    void setOperator();
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
    std::vector<ProcessWrapper*> processes;

signals:
    void accountsListed(const QList<TailAccountInfo>& accounts);
    void statusUpdated(TailStatus* newStatus);
    void loginFlowCompleted();
    void driveListed(const QList<TailDriveInfo>& drives, bool error, const QString& errorMsg);
    void fileSent(bool success, const QString& errorMsg, void* userData);

    void commandError(const QString& errorMsg, bool requiresSudoError);

private:
    void runCommand(Command cmdType, const QString& cmd, const QStringList& args, bool jsonResult = false, bool usePkExec = false, void* userData = nullptr);
    void parseStatusResponse(const QJsonObject& obj);

    [[nodiscard]] bool hasPendingCommandOfType(Command cmdType) const;
    void runCompletedCleanup();

private slots:
    void onProcessCanReadStdOut(const ProcessWrapper* wrapper);
    void onProcessCanReadStandardError(const ProcessWrapper* wrapper);
    void onProcessFinished(const ProcessWrapper* wrapper, int exitCode, QProcess::ExitStatus exitStatus);
};

#endif // TAILRUNNER_H
