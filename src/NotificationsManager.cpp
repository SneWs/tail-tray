#include "NotificationsManager.h"

#if defined(KNOTIFICATIONS_ENABLED)

#include <QDesktopServices>

NotificationsManager::NotificationsManager(QObject* parent)
    : QObject(parent)
{
    
}

NotificationsManager::~NotificationsManager()
{

}

void NotificationsManager::showNotification(const QString& title, const QString& message, const QVariant& data, const QString& iconName) {
    KNotification* notification = new KNotification("BasicNotification", KNotification::NotificationFlag::CloseOnTimeout, this);
    notification->setTitle(title);
    notification->setText(message);

    notification->setUrgency(KNotification::Urgency::HighUrgency);
    if (!iconName.isEmpty()) {
        notification->setIconName(iconName);
    }

    KNotificationAction* action = notification->addDefaultAction("View");
    connect(action, &KNotificationAction::activated, this, [this, notification, action]() {
        notification->close();

        notification->deleteLater();
        action->deleteLater();
    });
 
    notification->sendEvent();
}

void NotificationsManager::showFileNotification(const QString& title, const QString& message, const QString& filePath, const QVariant& data, const QString& iconName) {
    KNotification* notification = new KNotification("FileTransfer", KNotification::NotificationFlag::CloseOnTimeout, this);
    notification->setTitle(title);
    notification->setText(message);

    notification->setUrgency(KNotification::Urgency::HighUrgency);
    if (!iconName.isEmpty()) {
        notification->setIconName(iconName);
    }

    KNotificationAction* action = notification->addDefaultAction("Open Folder");
    connect(action, &KNotificationAction::activated, this, [this, notification, action, filePath]() {
        notification->close();
        QDesktopServices::openUrl(QUrl("file://" + filePath));

        notification->deleteLater();
        action->deleteLater();
    });

    notification->sendEvent();
}

#endif
