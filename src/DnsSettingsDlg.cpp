#include "DnsSettingsDlg.h"
#include "ui_DnsSettingsDlg.h"

DnsSettingsDlg::DnsSettingsDlg(const TailDnsStatus& pDnsStatus, bool dnsEnabled, QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::DnsSettingsDlg)
    , dnsStatus(pDnsStatus)
{
    ui->setupUi(this);

    connect(ui->btnClose, &QPushButton::clicked, this, &DnsSettingsDlg::close);
    connect(ui->chkUseTailscaleDns, &QCheckBox::toggled, this, &DnsSettingsDlg::dnsCheckStateChanged);

    // Sync the search domains and routes with the UI
    for (const auto& domain : dnsStatus.searchDomains)
        ui->lstSearchDomains->addItem(domain);
    
    for (const auto& route : dnsStatus.splitDnsRoutes)
        ui->lstRoutes->addItem(route.first + ": " + route.second);

    ui->chkUseTailscaleDns->setChecked(dnsEnabled);
    ui->grpSearchDomains->setVisible(dnsEnabled);
    ui->grpRoutes->setVisible(dnsEnabled);
}

DnsSettingsDlg::~DnsSettingsDlg()
{
    delete ui;
}

void DnsSettingsDlg::setTailscaleDnsEnabled(bool enabled) { 
    ui->chkUseTailscaleDns->setChecked(enabled);
}

bool DnsSettingsDlg::isTailscaleDnsEnabled() const {
    return ui->chkUseTailscaleDns->isChecked();
}

void DnsSettingsDlg::dnsCheckStateChanged(bool checked) {
    emit dnsEnabledChanged(checked);

    ui->grpSearchDomains->setVisible(checked);
    ui->grpRoutes->setVisible(checked);
}
