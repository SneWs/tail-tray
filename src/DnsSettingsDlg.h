#ifndef DNSSETTINGSDLG_H
#define DNSSETTINGSDLG_H

#include <QDialog>
#include <QList>

#include "models/Models.h"

namespace Ui {
class DnsSettingsDlg;
}

class DnsSettingsDlg : public QDialog
{
    Q_OBJECT
public:
    explicit DnsSettingsDlg(TailDnsStatus const* pDnsStatus, bool dnsEnabled, QWidget* parent = nullptr);
    ~DnsSettingsDlg();

    void setTailscaleDnsEnabled(bool enabled);
    bool isTailscaleDnsEnabled() const;

signals:
    void dnsEnabledChanged(bool enabled);

private slots:
    void dnsCheckStateChanged(Qt::CheckState state);

private:
    Ui::DnsSettingsDlg* ui;
    TailDnsStatus const* dnsStatus;
};

#endif // DNSSETTINGSDLG_H
