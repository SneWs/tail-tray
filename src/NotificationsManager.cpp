#include "NotificationsManager.h"

#include <QDesktopServices>

NotificationsManager::NotificationsManager(TrayMenuManager const* pTrayMgr, QObject* parent)
    : QObject(parent)
    , m_pTrayMgr(pTrayMgr)
{ }

NotificationsManager::~NotificationsManager() = default;

void NotificationsManager::showNotification(const QString& title, const QString& message, const QString& iconName) {
#if defined(KNOTIFICATIONS_ENABLED)
    auto* notification = new KNotification("BasicNotification", KNotification::NotificationFlag::CloseOnTimeout, this);
    notification->setTitle(title);
    notification->setText(message);

    notification->setUrgency(KNotification::Urgency::DefaultUrgency);
    if (!iconName.isEmpty()) {
        notification->setIconName(iconName);
    }
    else {
        notification->setIconName("notification-active");
    }

    notification->sendEvent();
#else
    m_pTrayMgr->trayIcon()->showMessage(title, message,
        QSystemTrayIcon::MessageIcon::Information, 5000);
#endif
}

void NotificationsManager::showFileNotification(const QString& title, const QString& message, const QFileInfo& fileInfo,
    const QString& iconName) {

#if defined(KNOTIFICATIONS_ENABLED)
    auto* notification = new KNotification("FileTransfer", KNotification::NotificationFlag::Persistent, this);
    notification->setTitle(title);
    notification->setText(message);

    // Setting the file URI will trigger the hamburger menu where one can select to open the file etc
    QUrl fileUrl("file://" + fileInfo.absoluteFilePath());
    notification->setUrls(QList{fileUrl});

    notification->setUrgency(KNotification::Urgency::DefaultUrgency);
    if (!iconName.isEmpty()) {
        notification->setIconName(iconName);
    }
    else {
        notification->setIconName("edit-image");
    }

    notification->sendEvent();
#else
    m_pTrayMgr->trayIcon()->showMessage(title, message,
        QSystemTrayIcon::MessageIcon::Information, 5000);
#endif
}

void NotificationsManager::showWarningNotification(const QString& title, const QString& message, const QString& iconName) {
#if defined(KNOTIFICATIONS_ENABLED)
    auto* notification = new KNotification("BasicNotification", KNotification::NotificationFlag::CloseOnTimeout, this);
    notification->setTitle(title);
    notification->setText(message);

    notification->setUrgency(KNotification::Urgency::DefaultUrgency);
    if (!iconName.isEmpty()) {
        notification->setIconName(iconName);
    }
    else {
        notification->setIconName("dialog-warning");
    }

    notification->sendEvent();
#else
    m_pTrayMgr->trayIcon()->showMessage(title, message, QSystemTrayIcon::Warning, 5000);
#endif
}

void NotificationsManager::showErrorNotification(const QString& title, const QString& message, const QString& iconName) {
#if defined(KNOTIFICATIONS_ENABLED)
    auto* notification = new KNotification("BasicNotification", KNotification::NotificationFlag::CloseOnTimeout, this);
    notification->setTitle(title);
    notification->setText(message);

    notification->setUrgency(KNotification::Urgency::DefaultUrgency);
    if (!iconName.isEmpty()) {
        notification->setIconName(iconName);
    }
    else {
        notification->setIconName("dialog-error");
    }

    notification->sendEvent();
#else
    m_pTrayMgr->trayIcon()->showMessage(title, message, QSystemTrayIcon::Critical, 5000);
#endif
}

void NotificationsManager::showNodeConnectedNotification(const QString& nodeName, const QString& ipAddress, const QString& os)
{
#if defined(KNOTIFICATIONS_ENABLED)
    auto* notification = new KNotification("NodeConnected", KNotification::NotificationFlag::Persistent, this);
    notification->setTitle(tr("Tailnet Devices"));
    notification->setText(tr("A new device have been discovered on your tailnet!\n\nDevice: %1 (%2) - %3")
        .arg(nodeName, ipAddress, os));

    notification->setIconName("online");
    notification->sendEvent();
#else
    m_pTrayMgr->trayIcon()->showMessage(tr("Tailnet Devices"),
    tr("A new device have been discovered on your tailnet!\n\nDevice: %1 (%2) - %3")
        .arg(nodeName, ipAddress, os), QSystemTrayIcon::Information, 8000);
#endif
}
void NotificationsManager::showNodeDisconnectedNotification(const QString& nodeName, const QString& ipAddress, const QString& os)
{
#if defined(KNOTIFICATIONS_ENABLED)
    auto* notification = new KNotification("NodeDisconnected", KNotification::NotificationFlag::Persistent, this);
    notification->setTitle(tr("Tailnet Devices"));
    notification->setText(tr("A device have been removed from your tailnet!\n\nDevice: %1 (%2) - %3")
        .arg(nodeName, ipAddress, os));

    notification->setIconName("offline");
    notification->sendEvent();
#else
    m_pTrayMgr->trayIcon()->showMessage(tr("Tailnet Devices"),
    tr("A device have been removed from your tailnet!\n\nDevice: %1 (%2) - %3")
        .arg(nodeName, ipAddress, os), QSystemTrayIcon::Information, 8000);
#endif
}