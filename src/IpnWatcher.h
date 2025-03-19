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
    void eventReceived();

private slots:
    void onProcessCanReadStdOut();
    void onProcessCanReadStandardError();
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);

private:
    std::unique_ptr<QProcess> m_pProcess;
    bool m_isShuttingDown;
};

#endif //IPNWATCHER_H
