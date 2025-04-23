#ifndef TAILRUNNER_H
#define TAILRUNNER_H

#include <memory>

#include <QProcess>

#include "models/Models.h"
#include "TailSettings.h"

enum class Command {
    SetOperator,
    SetExitNode,
    GetSettings,
    GetDnsStatus,
    SetSettings,
    ListAccounts,
    SwitchAccount,
    Login,
    Logout,
    Connect,
    Disconnect,
    SettingsChange,
    Status,
    SendFile,
    AdvertiseRoutes,
#if defined(DAVFS_ENABLED)
    Drive,
    DriveAdd,
    DriveRename,
    DriveRemove,
#endif
};

/// Simple way of abstracting away and be able to lazily delete and reference a process and it's states
class BufferedProcessWrapper : public QObject
{
    Q_OBJECT
public:
    explicit BufferedProcessWrapper(Command cmd, bool emitOnActualSignals = false, QObject* parent = nullptr);

    /// Start the process with the given command and arguments
    void start(const QString& cmd, QStringList args, bool jsonResult, bool usePkExec, void* userData);

    [[nodiscard]] QProcess* process() const { return proc.get();}
    [[nodiscard]] Command command() const { return eCommand; }
    [[nodiscard]] void* userData() const { return pUserData; }
    [[nodiscard]] bool isRunning() const { return proc != nullptr && proc->state() != QProcess::NotRunning; }

signals:
    void processCanReadStdOut(BufferedProcessWrapper* process);
    void processCanReadStandardError(BufferedProcessWrapper* process);
    void processFinished(BufferedProcessWrapper* process, int exitCode, QProcess::ExitStatus exitStatus);

private slots:
    void onProcessCanReadStdOut();
    void onProcessCanReadStandardError();
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);

private:
    bool bEmitOnActualSignals;
    std::unique_ptr<QProcess> proc;
    void* pUserData;
    Command eCommand;
    bool didReceiveStdErr;
    bool didReceiveStdOut;
};

class TailRunner : public QObject
{
    Q_OBJECT
public:
    explicit TailRunner(const TailSettings& s, QObject* parent = nullptr);
    ~TailRunner() override;

    void shutdown();

    void bootstrap();

    void readSettings();
    void readDnsStatus();
    void setOperator();
    void setExitNode(const QString& exitNode = "");
    void advertiseRoutes(const QList<QString>& definedRoutes);
    void applySettings(const TailSettings& s);
    void checkStatus();
    void getAccounts();
    void switchAccount(const QString& accountId);

    void login();
    void logout();

    void start(bool usePkExec = false);
    void stop();

#if defined(DAVFS_ENABLED)
    void listDrives();
    void addDrive(const TailDriveInfo& drive);
    void renameDrive(const TailDriveInfo& drive, const QString& newName);
    void removeDrive(const TailDriveInfo& drive);
#endif

    void sendFile(const QString& targetDevice, const QString& localFilePath, void* userData = nullptr);

    [[nodiscard]] const CurrentTailPrefs& currentSettings() const { return currentPrefs; }

private:
    const TailSettings& settings;
    CurrentTailPrefs currentPrefs;
    std::vector<BufferedProcessWrapper*> processes;

signals:
    void settingsRead();
    void dnsStatusRead(const TailDnsStatus& dnsStatus);
    void accountsListed(const QList<TailAccountInfo>& accounts);
    void statusUpdated(const TailStatus& newStatus);
    void loginFlowCompleted();
    void driveListed(const QList<TailDriveInfo>& drives, bool error, const QString& errorMsg);
    void fileSent(bool success, const QString& errorMsg, void* userData);

    void commandError(const QString& errorMsg, bool requiresSudoError);

private:
    void runCommand(Command cmdType, const QString& cmd, const QStringList& args, bool jsonResult = false, bool usePkExec = false, void* userData = nullptr);
    void parseStatusResponse(const QJsonObject& obj);
    void parseSettingsResponse(const QJsonObject& obj);

    [[nodiscard]] bool hasPendingCommandOfType(Command cmdType) const;
    void runCompletedCleanup();

private slots:
    void onProcessCanReadStdOut(const BufferedProcessWrapper* wrapper);
    void onProcessCanReadStandardError(const BufferedProcessWrapper* wrapper);
    void onProcessFinished(const BufferedProcessWrapper* wrapper, int exitCode, QProcess::ExitStatus exitStatus);
};

#endif // TAILRUNNER_H
