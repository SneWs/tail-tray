#ifndef NOTIFICATIONS_MANAGER
#define NOTIFICATIONS_MANAGER

#include <QObject>
#include <QList>
#include <QVariant>
#include <KNotification>

#if defined(KNOTIFICATIONS_ENABLED)

class NotificationsManager : public QObject
{
    Q_OBJECT
public:
    explicit NotificationsManager(QObject* parent = nullptr);
    ~NotificationsManager() override;

    void showNotification(const QString &title, const QString& message, const QVariant& data, const QString& iconName = QString());
    void showFileNotification(const QString& title, const QString& message, const QString& filePath, const QVariant& data, const QString& iconName = QString());
};

#endif // KNOTIFICATIONS_ENABLED

#endif // NOTIFICATIONS_MANAGER
