#include "ManageDriveWindow.h"

#include <QFileDialog>
#include <QPushButton>

ManageDriveWindow::ManageDriveWindow(const TailDriveInfo& drive, QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::ManageDriveWindow)
    , originalDriveInfo(drive)
{
    ui->setupUi(this);

    setWindowTitle("Tail drive");

    setWindowModality(Qt::WindowModality::ApplicationModal);
    setModal(true);
    driveToUi();

    connect(ui->btnSelectFolder, &QPushButton::clicked,
            this, &ManageDriveWindow::selectFolder);
}

void ManageDriveWindow::driveToUi() {
    ui->txtName->setText(originalDriveInfo.name);
    ui->txtPath->setText(originalDriveInfo.path);
}

TailDriveInfo ManageDriveWindow::driveInfo() const {
    TailDriveInfo retVal{};

    retVal.name = ui->txtName->text();
    retVal.path = ui->txtPath->text();

    return retVal;
}

void ManageDriveWindow::selectFolder() {
    QFileDialog dlg(this, "Select folder", ui->txtPath->text());
    dlg.setFileMode(QFileDialog::FileMode::Directory);
    dlg.setAcceptMode(QFileDialog::AcceptMode::AcceptOpen);
    dlg.setOption(QFileDialog::Option::ShowDirsOnly, true);

    auto result = dlg.exec();
    if (result == QFileDialog::Accepted) {
        const auto& selection = dlg.selectedFiles();
        ui->txtPath->setText(selection.first());
    }
}
