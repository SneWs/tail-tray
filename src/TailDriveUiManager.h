#ifndef TAILDRIVE_UI_MANAGER
#define TAILDRIVE_UI_MANAGER

// Only if told so by compiler flags
#if defined(DAVFS_ENABLED)

#include "TailRunner.h"
#include "TailSettings.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class TailDriveUiManager : public QObject
{
Q_OBJECT
public:
    explicit TailDriveUiManager(Ui::MainWindow* mainWndUi, TailRunner* runner, QObject* parent = nullptr);

    void stateChangedTo(TailState newState, const TailStatus& tailStatus);

private slots:
    void addTailDriveButtonClicked();
    void removeTailDriveButtonClicked();
    void selectTailDriveMountPath() const;
    void fixTailDriveDavFsSetup() const;

private:
    void tailDrivesToUi() const;
    [[nodiscard]] static bool isTailDriveFileAlreadySetup();

private:
    Ui::MainWindow* ui;
    TailRunner* pTailRunner;
    TailStatus pTailStatus;
    TailSettings settings;
};

#endif /* DAVFS_ENABLED */

#endif /* TAILDRIVE_UI_MANAGER */
