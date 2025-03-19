#ifndef IPNWATCHER_H
#define IPNWATCHER_H

#include <QProcess>

class IpnWatcher : public QObject {
    Q_OBJECT
public:
    explicit IpnWatcher(QObject* parent = nullptr);
    ~IpnWatcher() override;

    void start();
    void stop();

signals:
    /// This will signal any time a new event comes in
    /// TODO: We should scan for some specific well known event data types
    /// so we can provide better feedback to listeners
    /// but for now this will do just fine as the intended use case will be to re-read settings and check status
    /// etc a-new every time this is emitted
    void eventReceived();

private slots:
    void onProcessCanReadStdOut();
    void onProcessCanReadStandardError();
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);

private:
    std::unique_ptr<QProcess> m_process;
    bool m_isShuttingDown;
};

#endif // IPNWATCHER_H
