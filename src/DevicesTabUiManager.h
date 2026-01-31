#ifndef TAIL_TRAY_DEVICESTABUIMANAGER_H
#define TAIL_TRAY_DEVICESTABUIMANAGER_H

#include "TailRunner.h"
#include "ui_MainWindow.h"
#include "models/TailStatus.h"


class DevicesTabUiManager final : public QObject
{
    Q_OBJECT
public:
    explicit DevicesTabUiManager(TailRunner* currentExecution, Ui::MainWindow* _ui, QObject *parent = nullptr);
    ~DevicesTabUiManager() override = default;

    void syncToUi(const TailStatus& status);

private slots:
    void onDevicesTreeContextMenuRequested(const QPoint& pos);

signals:
    void ipAddressCopiedToClipboard(const QString& ipAddress, const QString& hostname);

private:
    Ui::MainWindow* ui;
    TailRunner* pCurrentExecution;
    TailStatus pTailStatus;
};


#endif //TAIL_TRAY_DEVICESTABUIMANAGER_H