#include "IpnWatcher.h"

IpnWatcher::IpnWatcher(QObject *parent)
    : QObject(parent)
    , m_process(nullptr)
    , m_isShuttingDown(false)
{
}

IpnWatcher::~IpnWatcher() {
    stop();
}

void IpnWatcher::start() {
    if (m_process != nullptr) {
        stop();
    }

    m_isShuttingDown = false;

    m_process = std::make_unique<QProcess>(this);
    connect(m_process.get(), &QProcess::finished,
        this, &IpnWatcher::onProcessFinished);

    connect(m_process.get(), &QProcess::readyReadStandardOutput,
        this, &IpnWatcher::onProcessCanReadStdOut);

    connect(m_process.get(), &QProcess::readAllStandardError,
        this, &IpnWatcher::onProcessCanReadStandardError);

    QStringList args;
    args << "debug";
    args << "watch-ipn";
    m_process->start("tailscale", args);
}

void IpnWatcher::stop() {
    m_isShuttingDown = true;
    if (m_process->state() == QProcess::Running) {
        m_process->terminate();
    }

    m_process->reset();
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
