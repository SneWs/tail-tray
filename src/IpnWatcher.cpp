#include "IpnWatcher.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>

#include "models/Models.h"

IpnWatcher::IpnWatcher(QObject *parent)
    : QObject(parent)
    , m_process(nullptr)
    , m_isShuttingDown(false)
{ }

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
    if (m_process == nullptr) {
        return;
    }

    if (m_process->state() == QProcess::Running) {
        m_process->terminate();
    }

    m_process.reset();
}

void IpnWatcher::onProcessCanReadStdOut() {
    QByteArray data = m_process->readAllStandardOutput();
    if (data.length() < 1) {
        return;
    }

    QString json = QString::fromUtf8(data);
    QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8());
    if (doc.isNull()) {
        return;
    }

    if (!doc.isObject()) {
        return;
    }

    QJsonObject obj = doc.object();
    qDebug() << "IPN Event received";

    IpnEventData eventData = IpnEventData::parse(obj);
    
    if (!eventData.Health.Warnings.networkStatus.Severity.isEmpty()) {
        qDebug() << "Nw" << (eventData.Health.Warnings.networkStatus.ImpactsConnectivity ? "Connectivity Disruption" : "Connection OK");
        qDebug() << "Severity: " << eventData.Health.Warnings.networkStatus.Severity;
        qDebug() << "Text: " << eventData.Health.Warnings.networkStatus.Text;
        qDebug() << "Title: " << eventData.Health.Warnings.networkStatus.Title;
    }

    emit eventReceived(eventData);
}

void IpnWatcher::onProcessCanReadStandardError() {
    //emit eventReceived();
}

void IpnWatcher::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    //emit eventReceived();

    if (!m_isShuttingDown) {
        start();
    }
}
