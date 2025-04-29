#ifndef NOTIFICATIONS_MANAGER
#define NOTIFICATIONS_MANAGER

#if defined(KNOTIFICATIONS_ENABLED)

#include <QList>
#include <QFileInfo>
#include <KNotification>

class NotificationsManager : public QObject
{
    Q_OBJECT
public:
    explicit NotificationsManager(QObject* parent = nullptr);
    ~NotificationsManager() override;

    void showNotification(const QString &title, const QString& message, const QVariant& data, const QString& iconName = QString());
    void showFileNotification(const QString& title, const QString& message, const QFileInfo& fileInfo,
        const QVariant& data, const QString& iconName = QString());
};

#endif // KNOTIFICATIONS_ENABLED

#endif // NOTIFICATIONS_MANAGER
