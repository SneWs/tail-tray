#include "NotificationsManager.h"

#include <QDesktopServices>

NotificationsManager::NotificationsManager(TrayMenuManager const* pTrayMgr, QObject* parent)
    : QObject(parent)
    , m_pTrayMgr(pTrayMgr)
{ }

NotificationsManager::~NotificationsManager() = default;

void NotificationsManager::showNotification(const QString& title, const QString& message, const QString& iconName) {
#if defined(KNOTIFICATIONS_ENABLED)
    auto* notification = new KNotification("BasicNotification", KNotification::NotificationFlag::Persistent, this);
    notification->setTitle(title);
    notification->setText(message);

    notification->setUrgency(KNotification::Urgency::HighUrgency);
    if (!iconName.isEmpty()) {
        notification->setIconName(iconName);
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

    notification->setUrgency(KNotification::Urgency::HighUrgency);
    if (!iconName.isEmpty()) {
        notification->setIconName(iconName);
    }

    notification->sendEvent();
#else
    m_pTrayMgr->trayIcon()->showMessage(title, message,
        QSystemTrayIcon::MessageIcon::Information, 5000);
#endif
}

void NotificationsManager::showWarningNotification(const QString& title, const QString& message, const QString& iconName) {
#if defined(KNOTIFICATIONS_ENABLED)
    m_pTrayMgr->trayIcon()->showMessage(title, message, QSystemTrayIcon::Warning, 5000);
#else
    m_pTrayMgr->trayIcon()->showMessage(title, message, QSystemTrayIcon::Warning, 5000);
#endif
}

void NotificationsManager::showErrorNotification(const QString& title, const QString& message, const QString& iconName) {
#if defined(KNOTIFICATIONS_ENABLED)
    m_pTrayMgr->trayIcon()->showMessage(title, message, QSystemTrayIcon::Critical, 5000);
#else
    m_pTrayMgr->trayIcon()->showMessage(title, message, QSystemTrayIcon::Critical, 5000);
#endif
}
