#ifndef ACCOUNTSTABUIMANAGER_H
#define ACCOUNTSTABUIMANAGER_H

#include "TailRunner.h"

#include <QMenu>
#include <memory>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class TailStatus;

class AccountsTabUiManager : public QObject
{
    Q_OBJECT
public:
    explicit AccountsTabUiManager(Ui::MainWindow* ui, TailRunner* runner, QObject* parent = nullptr);

    void onAccountsListed(const QList<TailAccountInfo>& foundAccounts);
    void onTailStatusChanged(const TailStatus& status);

private:
    void showAccountDetails(bool show = true);

private:
    Ui::MainWindow* ui;
    std::unique_ptr<QMenu> pAddAccountButtonMenu;
    TailRunner* pTailRunner;
    TailStatus pTailStatus;
    QList<TailAccountInfo> accounts;
    TailSettings settings;
};



#endif //ACCOUNTSTABUIMANAGER_H
