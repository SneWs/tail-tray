//
// Created by marcus on 2024-07-11.
//

#ifndef ACCOUNTSTABUIMANAGER_H
#define ACCOUNTSTABUIMANAGER_H

#include <QObject>

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
    explicit AccountsTabUiManager(Ui::MainWindow* ui, QObject* parent = nullptr);
    virtual ~AccountsTabUiManager();

    void onTailStatusChanged(TailStatus* pTailStatus);

private:
    Ui::MainWindow* ui;
};



#endif //ACCOUNTSTABUIMANAGER_H
