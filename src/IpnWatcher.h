#ifndef IPNWATCHER_H
#define IPNWATCHER_H

#include <QProcess>

#include "models/Models.h"

class IpnWatcher : public QObject {
    Q_OBJECT
public:
    explicit IpnWatcher(QObject* parent = nullptr);
    ~IpnWatcher() override;

    void start();
    void stop();

signals:
    /// This will signal any time a new event comes in from the IPN watcher.
    /// The receiver needs to take ownership of the event data.
    void eventReceived(const IpnEventData& eventData);

private slots:
    void onProcessCanReadStdOut();
    void onProcessCanReadStandardError();
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);

private:
    std::unique_ptr<QProcess> m_process;
    bool m_isShuttingDown;
};

#endif // IPNWATCHER_H
