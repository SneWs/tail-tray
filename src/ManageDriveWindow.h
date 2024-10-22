#ifndef MANAGEDRIVEWINDOW_H
#define MANAGEDRIVEWINDOW_H

#include <memory>

#include <QDialog>
#include <QAction>
#include "ui_ManageDriveWindow.h"

#include "models.h"

class ManageDriveWindow : public QDialog
{
    Q_OBJECT
public:
    explicit ManageDriveWindow(const TailDriveInfo& drive, QWidget* parent = nullptr);

    [[nodiscard]] TailDriveInfo driveInfo() const;

private:
    std::unique_ptr<Ui::ManageDriveWindow> ui;
    TailDriveInfo originalDriveInfo;

    void driveToUi();

private slots:
    void selectFolder();
    void acceptButtonClicked();
};

#endif // MANAGEDRIVEWINDOW_H
