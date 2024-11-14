#ifndef TAILUSER_H
#define TAILUSER_H

#include <QObject>
#include <QString>
#include <QJsonObject>
#include <QJsonArray>

#include <memory>
#include <utility>

class TailUser final : public QObject
{
    Q_OBJECT
public:
    long long id;
    QString loginName;
    QString displayName;
    QString profilePicUrl;
    QList<QString> roles;

    static std::unique_ptr<TailUser> parse(const QJsonObject& obj, long long userId) {
        auto user = std::make_unique<TailUser>();

        // Get the user object based on the user id (user id comes from the self part in the same doc)
        auto thisUser = obj[QString::number(userId)].toObject();

        user->id = thisUser["ID"].toInteger(0L);
        user->loginName = thisUser["LoginName"].toString("");
        user->displayName = thisUser["DisplayName"].toString("");
        user->profilePicUrl = thisUser["ProfilePicURL"].toString("");

        for (const auto& ab : thisUser["roles"].toArray()) {
            user->roles.emplace_back(ab.toString(""));
        }

        return user;
    }
};

#endif //TAILUSER_H
