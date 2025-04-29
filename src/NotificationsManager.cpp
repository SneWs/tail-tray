#include "NotificationsManager.h"

#if defined(KNOTIFICATIONS_ENABLED)

#include <QDesktopServices>

NotificationsManager::NotificationsManager(QObject* parent)
    : QObject(parent)
{ }

NotificationsManager::~NotificationsManager() = default;

void NotificationsManager::showNotification(const QString& title, const QString& message, const QVariant& data, const QString& iconName) {
    auto* notification = new KNotification("BasicNotification", KNotification::NotificationFlag::Persistent, this);
    notification->setTitle(title);
    notification->setText(message);

    notification->setUrgency(KNotification::Urgency::HighUrgency);
    if (!iconName.isEmpty()) {
        notification->setIconName(iconName);
    }

    notification->sendEvent();
}

void NotificationsManager::showFileNotification(const QString& title, const QString& message, const QFileInfo& fileInfo,
    const QVariant& data, const QString& iconName) {

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
}

#endif
