#include "TailDriveUiManager.h"
#include <QMessageBox>
#include <QFile>
#include <QFileDialog>

#include "KnownValues.h"
#include "ManageDriveWindow.h"
#include "MainWindow.h"

#if defined(DAVFS_ENABLED)

TailDriveUiManager::TailDriveUiManager(Ui::MainWindow* mainWndUi, TailRunner* runner, QObject* parent)
    : QObject(parent)
    , ui(mainWndUi)
    , pTailRunner(runner)
    , pTailStatus(nullptr)
    , settings(this)
{
    connect(ui->btnAddTailDrive, &QPushButton::clicked,
        this, &TailDriveUiManager::addTailDriveButtonClicked);

    connect(ui->btnRemoveSelectedTailDrive, &QPushButton::clicked,
            this, &TailDriveUiManager::removeTailDriveButtonClicked);

    connect(ui->chkUseTailDrive, &QCheckBox::clicked,
            this, [this]() {
                auto checked = ui->chkUseTailDrive->isChecked();
                settings.tailDriveEnabled(checked);

                if (checked) {
                    ui->tabWidget->insertTab(3, ui->tabTailDrive, QIcon::fromTheme("drive-removable-media"), "Tail Drive");
                }
                else {
                    ui->tabWidget->removeTab(3);
                }
            });

    connect(ui->btnSelectTailDriveMountPath, &QPushButton::clicked,
            this, &TailDriveUiManager::selectTailDriveMountPath);

    if (!isTailDriveFileAlreadySetup()) {
        ui->btnTailDriveFixDavFsMountSetup->setEnabled(true);
        ui->btnTailDriveFixDavFsMountSetup->setText(tr("Fix it for me"));
        connect(ui->btnTailDriveFixDavFsMountSetup, &QPushButton::clicked,
                this, &TailDriveUiManager::fixTailDriveDavFsSetup);
    }
    else {
        ui->btnTailDriveFixDavFsMountSetup->setEnabled(false);
        ui->btnTailDriveFixDavFsMountSetup->setText(tr("Configured and ready"));
    }
}

void TailDriveUiManager::stateChangedTo(TailState newState, TailStatus* tailStatus) {
    pTailStatus = tailStatus;
    tailDrivesToUi();
}

void TailDriveUiManager::addTailDriveButtonClicked() const {
    ManageDriveWindow dlg(TailDriveInfo{}, nullptr);
    auto result = dlg.exec();
    if (result == QDialog::Accepted) {
        auto newDrive = dlg.driveInfo();
        pTailRunner->addDrive(newDrive);

        pTailStatus->drives.emplace_back(newDrive);
        tailDrivesToUi();
    }
}

void TailDriveUiManager::removeTailDriveButtonClicked() const {
    auto selectedItems = ui->twSharedDrives->selectedItems();
    if (selectedItems.count() < 1) {
        return;
    }

    auto row = ui->twSharedDrives->row(selectedItems.first());
    const auto& drive = pTailStatus->drives[row];

    auto answer = QMessageBox::question(nullptr,
        tr("Are you sure?"),
        tr("Do you really want to remove the share ") + drive.path + "?",
        QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);

    if (answer != QMessageBox::Yes) {
        return;
    }

    pTailRunner->removeDrive(drive);

    pTailStatus->drives.removeAt(row);
    ui->twSharedDrives->removeRow(row);
    if (row > 0)
        ui->twSharedDrives->selectRow(row - 1);

    tailDrivesToUi();
}

void TailDriveUiManager::selectTailDriveMountPath() const {
    QFileDialog dlg(nullptr, tr("Select mount path"), ui->txtTailDriveDefaultMountPath->text());
    dlg.setOption(QFileDialog::Option::ShowDirsOnly, true);
    dlg.setFileMode(QFileDialog::FileMode::Directory);

    auto result = dlg.exec();
    if (result == QDialog::Accepted) {
        auto files = dlg.selectedFiles();
        if (files.count() > 0) {
            ui->txtTailDriveDefaultMountPath->setText(files.first());
        }
    }
}

void TailDriveUiManager::fixTailDriveDavFsSetup() const {
    auto homeDir = KnownValues::getHomeDir();
    auto homeDavFsSecret = KnownValues::getTailDriveFilePath();

    if (isTailDriveFileAlreadySetup())
        return;

    // Create the .davfs2 folder if it doesn't exist
    auto davDir = QDir(homeDir);
    (void)davDir.mkpath(".davfs2"); // Don't care for return val

    QFile davFsSecret(homeDavFsSecret);
    davFsSecret.open(QIODevice::ReadWrite);

    // We need to add our config lines
    davFsSecret.seek(davFsSecret.size());
    davFsSecret.write(QString("\n# Tailscale davfs server config\n").toUtf8());
    davFsSecret.write(QString(KnownValues::tailDavFsUrl + "\tGuest\tGuest\n").toUtf8());

    davFsSecret.close();

    QMessageBox::information(nullptr, "Tail Tray", tr("davfs2 config has been written"));

    ui->btnTailDriveFixDavFsMountSetup->setEnabled(isTailDriveFileAlreadySetup());
}

void TailDriveUiManager::tailDrivesToUi() const {
    if (pTailStatus == nullptr) {
        return;
    }

    const auto& drives = pTailStatus->drives;

    ui->twSharedDrives->clearContents();
    ui->twSharedDrives->setColumnCount(2);
    ui->twSharedDrives->setHorizontalHeaderLabels(QStringList() << tr("Name") << tr("Path"));
    ui->twSharedDrives->setRowCount(static_cast<int>(drives.count()));

    for (int i = 0; i < drives.count(); i++) {
        const auto& drive = drives[i];
        qDebug() << "Drive: " << drive.name << " (" << drive.path << ")";

        ui->twSharedDrives->setItem(i, 0, new QTableWidgetItem(drive.name));
        ui->twSharedDrives->setItem(i, 1, new QTableWidgetItem(drive.path));
    }
}

bool TailDriveUiManager::isTailDriveFileAlreadySetup() {
#if defined(WINDOWS_BUILD)
    return true;
#endif

    auto homeDavFsSecret = KnownValues::getTailDriveFilePath();

    // Create the .davfs2 folder if it doesn't exist
    QFile davFsSecret(homeDavFsSecret);
    if (!davFsSecret.open(QIODevice::ReadWrite)) {
        davFsSecret.close();
        return false;
    }

    auto fileContent = QString(davFsSecret.readAll());
    auto lines = fileContent.split('\n', Qt::SkipEmptyParts);
    for (const auto& line : lines) {
        if (line.trimmed().startsWith('#'))
            continue; // Comment
        if (line.contains(KnownValues::tailDavFsUrl, Qt::CaseInsensitive)) {
            return true;
        }
    }

    return false;
}

#endif /* DAVFS_ENABLED */