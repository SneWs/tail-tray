#ifndef NOTIFICATIONS_MANAGER
#define NOTIFICATIONS_MANAGER

#include <QFileInfo>

#if defined(KNOTIFICATIONS_ENABLED)
#include <KNotification>
#endif

#include "TrayMenuManager.h"

enum class NotificationLevel {
    Info,
    Warning,
    Error,
    Critical,
};

class NotificationsManager : public QObject
{
    Q_OBJECT
public:
    explicit NotificationsManager(TrayMenuManager const* pTrayMgr, QObject* parent = nullptr);
    ~NotificationsManager() override;

    // --- typed helpers (preferred call sites) ---
    void showNotification(const QString &title, const QString& message, const QString& iconName = QString());
    void showFileNotification(const QString& title, const QString& message, const QFileInfo& fileInfo,
        const QString& iconName = QString());

    void showWarningNotification(const QString& title, const QString& message, const QString& iconName = QString("dialog-warning"));
    void showErrorNotification(const QString& title, const QString& message, const QString& iconName = QString("dialog-error"));
    void showCriticalNotification(const QString& title, const QString& message, const QString& iconName = QString("dialog-error"));

    void showNodeConnectedNotification(const QString& nodeName, const QString& ipAddress, const QString& os);
    void showNodeDisconnectedNotification(const QString& nodeName, const QString& ipAddress, const QString& os);

    // --- generic entry-point driven by a NotificationLevel flag ---
    void showLevelNotification(NotificationLevel level, const QString& title, const QString& message);

private:
    TrayMenuManager const* m_pTrayMgr;
};

#endif // NOTIFICATIONS_MANAGER
