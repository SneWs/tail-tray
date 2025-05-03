#ifndef NOTIFICATIONS_MANAGER
#define NOTIFICATIONS_MANAGER

#include <QFileInfo>

#if defined(KNOTIFICATIONS_ENABLED)
#include <KNotification>
#endif

#include "TrayMenuManager.h"

class NotificationsManager : public QObject
{
    Q_OBJECT
public:
    explicit NotificationsManager(TrayMenuManager const* pTrayMgr, QObject* parent = nullptr);
    ~NotificationsManager() override;

    void showNotification(const QString &title, const QString& message, const QString& iconName = QString());
    void showFileNotification(const QString& title, const QString& message, const QFileInfo& fileInfo,
        const QString& iconName = QString());

    void showWarningNotification(const QString& title, const QString& message, const QString& iconName = QString("dialog-warning"));
    void showErrorNotification(const QString& title, const QString& message, const QString& iconName = QString("dialog-error"));

private:
    TrayMenuManager const* m_pTrayMgr;
};

#endif // NOTIFICATIONS_MANAGER
