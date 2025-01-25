#ifndef TAILUSER_H
#define TAILUSER_H

#include <QObject>
#include <QString>
#include <QJsonObject>
#include <QJsonArray>

#include <memory>
#include <utility>

#include "JsonHelpers.h"

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

        auto idKey = QString::number(userId);
        if (!obj.contains(idKey) || obj[idKey].isNull()) {
            return user;
        }

        // Get the user object based on the user id (user id comes from the self part in the same doc)
        auto thisUser = obj[idKey].toObject();

        user->id = userId;

        user->loginName = safeReadStr(thisUser, "LoginName");
        user->displayName = safeReadStr(thisUser, "DisplayName");
        user->profilePicUrl = safeReadStr(thisUser, "ProfilePicUrl");

        if (thisUser.contains("roles"))
        {
            for (const auto& ab : thisUser["roles"].toArray()) {
                if (ab.isNull())
                    continue;

                user->roles.emplace_back(ab.toString(""));
            }
        }

        return user;
    }
};

#endif //TAILUSER_H
