#include "ManageDriveWindow.h"

#if defined(DAVFS_ENABLED)

#include <QFileDialog>
#include <QMessageBox>
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

    connect(ui->btnOk, &QPushButton::clicked,
            this, &ManageDriveWindow::acceptButtonClicked);

    connect(ui->btnCancel, &QPushButton::clicked,
            this, &ManageDriveWindow::close);
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
    QFileDialog dlg(this, tr("Select folder"), ui->txtPath->text());
    dlg.setFileMode(QFileDialog::FileMode::Directory);
    dlg.setAcceptMode(QFileDialog::AcceptMode::AcceptOpen);
    dlg.setOption(QFileDialog::Option::ShowDirsOnly, true);

    auto result = dlg.exec();
    if (result == QFileDialog::Accepted) {
        const auto& selection = dlg.selectedFiles();
        ui->txtPath->setText(selection.first());
        if (ui->txtName->text().isEmpty()) {
            const QDir dir(selection.first());
            ui->txtName->setText(dir.dirName());
        }
    }
}

void ManageDriveWindow::acceptButtonClicked() {
    QString error;
    if (ui->txtName->text().isEmpty()) {
        error = tr("Name must be set");
        setResult(QDialog::Rejected);
    }

    const QDir dir(ui->txtPath->text());
    if (ui->txtPath->text().isEmpty() || !dir.exists()) {
        error = tr("Path must be set and exist");
        setResult(QDialog::Rejected);
    }

    if (!error.isEmpty()) {
        QMessageBox::warning(this, tr("Error"), error, QMessageBox::Ok);
        return;
    }

    setResult(QDialog::Accepted);
    accept();
}

#endif
