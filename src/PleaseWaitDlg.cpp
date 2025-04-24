#include "PleaseWaitDlg.h"
#include "ui_PleaseWaitDlg.h"

PleaseWaitDlg::PleaseWaitDlg(const QString& message, QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::PleaseWaitDlg)
{
    ui->setupUi(this);
    ui->lblMessage->setText(message);

    connect(ui->btnCancel, &QPushButton::clicked,
        this, &PleaseWaitDlg::onCancelButtonClicked);
}

PleaseWaitDlg::~PleaseWaitDlg() {
    delete ui;
}

void PleaseWaitDlg::setMessage(const QString &text) const {
    ui->lblMessage->setText(text);
}

void PleaseWaitDlg::onCancelButtonClicked() {
    emit userCancelled();
    close();
}
