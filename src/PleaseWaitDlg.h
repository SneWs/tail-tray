#ifndef PLEASEWAITDLG_H
#define PLEASEWAITDLG_H

#include <QDialog>

namespace Ui {
class PleaseWaitDlg;
}

class PleaseWaitDlg : public QDialog
{
    Q_OBJECT

public:
    explicit PleaseWaitDlg(const QString& message, QWidget* parent = nullptr);
    ~PleaseWaitDlg() override;

    void setMessage(const QString& text) const;

signals:
    void userCancelled();

private slots:
    void onCancelButtonClicked();

private:
    Ui::PleaseWaitDlg* ui;
};

#endif // PLEASEWAITDLG_H
