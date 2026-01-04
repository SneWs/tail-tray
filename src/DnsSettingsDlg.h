#ifndef DNSSETTINGSDLG_H
#define DNSSETTINGSDLG_H

#include <QDialog>

#include "models/Models.h"

namespace Ui {
class DnsSettingsDlg;
}

class DnsSettingsDlg : public QDialog
{
    Q_OBJECT
public:
    explicit DnsSettingsDlg(const TailDnsStatus& pDnsStatus, bool dnsEnabled, QWidget* parent = nullptr);
    ~DnsSettingsDlg() override;

    void setTailscaleDnsEnabled(bool enabled) const;
    [[nodiscard]] bool isTailscaleDnsEnabled() const;

signals:
    void dnsEnabledChanged(bool enabled);

private slots:
    void dnsCheckStateChanged(bool checked);

private:
    Ui::DnsSettingsDlg* ui;
    TailDnsStatus dnsStatus;
};

#endif // DNSSETTINGSDLG_H
