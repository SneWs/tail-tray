#include "IpnWatcher.h"

IpnWatcher::IpnWatcher(QObject *parent)
    : QObject(parent)
    , m_pProcess(nullptr)
    , m_isShuttingDown(false)
{
}

IpnWatcher::~IpnWatcher() {
    stop();
}

void IpnWatcher::start() {
    if (m_pProcess != nullptr) {
        stop();
    }

    m_isShuttingDown = false;

    m_pProcess = std::make_unique<QProcess>(this);
    connect(m_pProcess.get(), &QProcess::finished,
        this, &IpnWatcher::onProcessFinished);

    connect(m_pProcess.get(), &QProcess::readyReadStandardOutput,
        this, &IpnWatcher::onProcessCanReadStdOut);

    connect(m_pProcess.get(), &QProcess::readAllStandardError,
        this, &IpnWatcher::onProcessCanReadStandardError);

    QStringList args;
    args << "debug";
    args << "watch-ipn";
    m_pProcess->start("tailscale", args);
}

void IpnWatcher::stop() {
    m_isShuttingDown = true;
    if (m_pProcess->state() == QProcess::Running) {
        m_pProcess->terminate();
    }

    m_pProcess->reset();
}

void IpnWatcher::onProcessCanReadStdOut() {
    emit eventReceived();
}

void IpnWatcher::onProcessCanReadStandardError() {
    emit eventReceived();
}

void IpnWatcher::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    emit eventReceived();

    if (!m_isShuttingDown) {
        start();
    }
}
